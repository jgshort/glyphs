#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static const int point_size = 64;

typedef struct {
  SDL_Texture * texture;
  char * c; /* one or more UTF-8 code points */
  int w;
  int h;
} glyph;

typedef struct {
  TTF_Font * font;

  /* TTF_Font exposes a number of properties; we'll save a few for later. */
  int height;
  char padding[4];

  /* Text storage for graphemes; simplified here */
  size_t text_buf_len;
  size_t text_buf_capacity;
  char * text_buf;
  char * text_next;

  /* the glyph atlas */
  size_t glyphs_capacity;
  size_t glyphs_len;
  glyph * glyphs;
} font_data;

typedef struct font font;
typedef struct font {
  const glyph * (*putchar)(
      font * self,
      SDL_Renderer * renderer,
      SDL_Point dest,
      char text
      );
  void (*write)(
      font * self,
      SDL_Renderer * renderer,
      SDL_Point point,
      const char * text
      );
  font_data data;
} font;

static const glyph * font_putchar(font * self, SDL_Renderer * renderer, SDL_Point dest, char text);
static void font_init(font * self, SDL_Renderer * renderer, TTF_Font * ttf);
static void font_build_atlas(font * self, TTF_Font * ttf, SDL_Renderer * renderer);
static void font_glyph_create_texture(TTF_Font * ttf, SDL_Renderer * renderer, glyph * g);
static void font_write(font * self, SDL_Renderer * renderer, SDL_Point point, const char * text);

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  if(argc <= 1) { fprintf(stderr, "Provide a path to a TTF font.\n"); exit(-1); }

  const char * font_path = argv[1];

  static const double SP_HERTZ = 30.0;
  static const uint32_t SP_TARGET_FPS = 60;
  static const uint32_t SP_MILLI = 1000;

  const uint64_t SP_TIME_BETWEEN_UPDATES = (uint64_t)(SP_MILLI / SP_HERTZ);
  static const int SP_MAX_UPDATES_BEFORE_RENDER = 5;
  static const int SP_TARGET_TIME_BETWEEN_RENDERS = SP_MILLI / SP_TARGET_FPS;

  uint64_t now = 0;
  uint64_t last_render_time = SDL_GetTicks64();
  uint64_t last_update_time = SDL_GetTicks64();

  const uint32_t window_flags
    = SDL_WINDOW_OPENGL
    | SDL_WINDOW_HIDDEN
    | SDL_WINDOW_ALLOW_HIGHDPI
    ;
  const uint32_t renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  SDL_Window * window = SDL_CreateWindow("glyphs", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, window_flags);

  SDL_SetWindowSize(window, 1024, 768);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  const int default_driver = -1;
  SDL_Renderer * renderer = SDL_CreateRenderer(window, default_driver, renderer_flags);

  TTF_Font * ttf = TTF_OpenFont(font_path, point_size);
  if(!ttf) { fprintf(stderr, "%s\n", SDL_GetError()); exit(-1); }

  font f;
  font_init(&f, renderer, ttf);

  SDL_Event evt = { 0 };
  bool is_running = true;
  while(is_running) {
    int update_loops = 0;
    now = SDL_GetTicks64();
    while((now - last_update_time > SP_TIME_BETWEEN_UPDATES && update_loops < SP_MAX_UPDATES_BEFORE_RENDER)) {
      while(SDL_PollEvent(&evt) > 0) {
        switch(evt.type) {
          case SDL_QUIT:
            goto end_of_running_loop;
            break;
          case SDL_KEYDOWN:
            {
              SDL_Keycode sym = evt.key.keysym.sym;
              switch(sym) {
                case SDLK_q:
                  /* ctrl-q to quit */
                  if((SDL_GetModState() & KMOD_CTRL) != 0) {
                    is_running = false;
                    goto end_of_running_loop;
                  }
                  break;
                default:
                  break;
              }
            }
            break;
          default:
            break;
        }
      }

      last_update_time += SP_TIME_BETWEEN_UPDATES;
      update_loops++;
    }

    if (now - last_update_time > SP_TIME_BETWEEN_UPDATES) {
      last_update_time = now - SP_TIME_BETWEEN_UPDATES;
    }

    SDL_RenderClear(renderer);

    SDL_Point p = { .x = 100, .y = 100 };
    f.write(&f, renderer, p, "hello, world!");

    SDL_RenderPresent(renderer);

    while (now - last_render_time < SP_TARGET_TIME_BETWEEN_RENDERS
        && now - last_update_time < SP_TIME_BETWEEN_UPDATES
    ) {
      SDL_Delay(0);
      now = SDL_GetTicks64();
    }
  }

end_of_running_loop:
  for(size_t i = 0; i < f.data.glyphs_len; i++) {
    glyph * g = &f.data.glyphs[i];
    if(g->texture) { SDL_DestroyTexture(g->texture); }
  }
  free(f.data.glyphs), f.data.glyphs = NULL;
  free(f.data.text_buf), f.data.text_buf = NULL;
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  return 0;
}

static void font_init(font * self, SDL_Renderer * renderer, TTF_Font * ttf) {
  font_data * data = &(self->data);

  self->putchar = &font_putchar;
  self->write = &font_write;

  data->font = ttf;
  data->height = TTF_FontHeight(ttf);

  data->glyphs_len = 0;
  data->glyphs_capacity = 256;
  data->text_buf_len = 0;
  data->text_buf_capacity = 2048;

  data->glyphs = calloc(data->glyphs_capacity, sizeof * data->glyphs);
  if(!data->glyphs) { abort(); }

  data->text_buf = calloc(data->text_buf_capacity, sizeof * data->text_buf);
  if(!data->text_buf) { abort(); }
  data->text_next = data->text_buf;

  font_build_atlas(self, ttf, renderer);
}

static const glyph * font_putchar(font * self, SDL_Renderer * renderer, SDL_Point dest, char text) {
  if(text == '\0') { return NULL; }

  const glyph * g = &self->data.glyphs[(size_t)text];
  SDL_Rect d = {
    .x = dest.x,
    .y = dest.y,
    .w = g->w,
    .h = self->data.height
  };

  SDL_RenderCopy(renderer, g->texture, NULL, &d);

  return g;
}

static void font_write(font * self, SDL_Renderer * renderer, SDL_Point point, const char * text) {
  const char * t = text;
  do {
    const glyph * g = self->putchar(self, renderer, point, *t);
    point.x += g->w;
  } while(t++, *t != '\0');
}

static void font_build_atlas(font * self, TTF_Font * ttf, SDL_Renderer * renderer) {
  font_data * data = &(self->data);
  for(size_t i = 0; i < data->glyphs_capacity; i++) {
    if(data->text_buf_len + 1 >= data->text_buf_capacity) { abort(); }

    glyph * g = data->glyphs + i;
    g->c = data->text_next;
    g->c[0] = (char)i; /* laziness */
    g->c[1] = '\0';

    data->text_next += sizeof * g->c + sizeof '\0';
    data->text_buf_len++;

    font_glyph_create_texture(ttf, renderer, g);

    TTF_SizeUTF8(data->font, g->c, &(g->w), &(g->h));
    data->glyphs_len++;
  }
}

static void font_glyph_create_texture(TTF_Font * ttf, SDL_Renderer * renderer, glyph * g) {
  static const SDL_Color foreground = { .r = 255, .g = 255, .b = 255, .a = 255 };
  if(!g->c || *(g->c) == '\0') { return; }
  SDL_Surface * surface = TTF_RenderUTF8_Blended(ttf, g->c, foreground);
  g->texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface), surface = NULL;
}

