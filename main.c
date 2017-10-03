#include <SDL2/SDL.h>

typedef struct {
    int width;
    int height;
    Uint32 * pixels;
} T_CANVAS;

typedef struct {
    int x;
    int y;
} T_POINT;


typedef struct {
    Uint8 r;
    Uint8 g; 
    Uint8 b;
} T_RGB;

enum T_MODE {
    EDIT,
    PREVIEW
};

typedef struct {
    int scale;
    T_POINT cursor;
    T_RGB cursor_color;
    T_POINT dimension; 
    unsigned char show_grid;
    T_POINT start;
    T_CANVAS * canvas_list[9];
    int active_canvas;
    enum T_MODE mode;
} T_WORKSPACE;

static T_RGB RGB_WHITE;
static T_RGB RGB_BLACK;
static T_WORKSPACE g_workspace;

T_CANVAS * create_canvas(int width, int height) {
    T_CANVAS * canvas = (T_CANVAS*)malloc(sizeof(T_CANVAS));
    canvas->width = 64;
    canvas->height = 64;
    canvas->pixels = (Uint32*)calloc(width*height,sizeof(Uint32));
    return canvas;
}

void init() {
    RGB_WHITE.r = 0xff;
    RGB_WHITE.g = 0xff;
    RGB_WHITE.b = 0xff;    

    RGB_BLACK.r = 0x0;
    RGB_BLACK.g = 0x0;
    RGB_BLACK.b = 0x0;

    g_workspace.scale = 1;
    g_workspace.cursor.x = 0;
    g_workspace.cursor.y = 0;

    g_workspace.cursor_color.r = 0xFF;
    g_workspace.cursor_color.g = 0x0F;
    g_workspace.cursor_color.b = 0x0F;

    g_workspace.show_grid = 1;

    g_workspace.canvas_list[0] = create_canvas(64,64);
    for (int i = 1;i <  9;i++) g_workspace.canvas_list[i] = NULL;
    g_workspace.active_canvas = 0;

    g_workspace.start.x = 0;
    g_workspace.start.y = 0;

    g_workspace.mode = EDIT;
}

void draw_hline(T_POINT start, int length, T_RGB color, SDL_Surface * surface) {
    Uint32 u32color = SDL_MapRGB(surface->format, color.r, color.g, color.b);
    int offset = start.y*surface->w + start.x;
    Uint32 * pixels = (Uint32 *)surface->pixels;
    for (int i = 0;i < length;i++) pixels[offset++] = u32color;
}

void draw_vline(T_POINT start, int length, T_RGB color, SDL_Surface * surface) {
    Uint32 u32color = SDL_MapRGB(surface->format, color.r, color.g, color.b);
    int offset = start.y*surface->w + start.x;
    Uint32 * pixels = (Uint32 *)surface->pixels;
    for (int i = 0;i < length;i++) {
        pixels[offset] = u32color;
        offset += surface->w; 
    } 
}

void draw_grid(int step, T_RGB color, SDL_Surface * surface) {
    for (int i = 0;i <= surface->w/step;i++) {
        T_POINT p;
        p.x = i * step;
        p.y = 0;
        if(i * step < surface->w) draw_vline(p,surface->h,color,surface);
    }
    for (int j = 0;j <= surface->h/step;j++) {
        T_POINT p;
        p.x = 0;
        p.y = j * step;
        if(j * step < surface->h) draw_hline(p,surface->w,color,surface);
    }
}

void draw_cursor(T_WORKSPACE workspace, SDL_Surface * surface) {
    SDL_Rect rect;
    rect.x = (workspace.cursor.x-workspace.start.x) * workspace.scale;
    rect.y = (workspace.cursor.y-workspace.start.y) * workspace.scale;
    rect.w = workspace.scale;
    rect.h = workspace.scale;
    SDL_FillRect( surface, &rect, SDL_MapRGB( surface->format, workspace.cursor_color.r, workspace.cursor_color.g, workspace.cursor_color.b ) );    

}

void draw_canvas(T_WORKSPACE workspace,T_CANVAS canvas, SDL_Surface * surface) {
    SDL_Rect rect;
    rect.w = workspace.scale;
    rect.h = workspace.scale;
    for(int i = workspace.start.x;i < workspace.start.x+surface->w/workspace.scale;i++)
        for(int j = workspace.start.y;j < workspace.start.y+surface->h/workspace.scale;j++) {
            if ((i >= 0) && (i<canvas.width)&&(j >= 0) && (j<canvas.height)) {
                int offset = j*canvas.width+i;
                rect.x = (i-workspace.start.x) * workspace.scale;
                rect.y = (j-workspace.start.y) * workspace.scale;            
                Uint32 color = canvas.pixels[offset];
                SDL_FillRect( surface, &rect, color );    
            }
        }
}


SDL_Surface * createSurface(int width, int height) {
    /* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
    as expected by OpenGL for textures */
    SDL_Surface *surface;
    Uint32 rmask, gmask, bmask, amask;
   
    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
    on the endianness (byte order) of the machine */
   #if SDL_BYTEORDER == SDL_BIG_ENDIAN
       rmask = 0xff000000;
       gmask = 0x00ff0000;
       bmask = 0x0000ff00;
       amask = 0x000000ff;
   #else
       rmask = 0x000000ff;
       gmask = 0x0000ff00;
       bmask = 0x00ff0000;
       amask = 0xff000000;
   #endif
   
    surface = SDL_CreateRGBSurface(0, width, height, 32,
                                    rmask, gmask, bmask, amask);
    if (surface == NULL) {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
        exit(1);
    }      
}

void draw(SDL_Window * window) {
    SDL_Surface * screenSurface = SDL_GetWindowSurface( window ); 
    
    SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0x08, 0x08, 0x08 ) );

    draw_canvas(g_workspace,*g_workspace.canvas_list[g_workspace.active_canvas], screenSurface);
    if (g_workspace.show_grid) draw_grid(g_workspace.scale,RGB_WHITE,screenSurface);        
    draw_cursor(g_workspace,screenSurface);
    SDL_UpdateWindowSurface(window);
}



int main(int argc, char* argv[])
{
    init();
    int exit = 0;
    SDL_Window *window;

    int width = 640;
    int height = 480;
    int screenflags = 0;
    for (int i = 1;i < argc;i++) {
        switch(argv[i][0]) {
            case 'w' : width = atoi(&argv[i][1]);break;
            case 'h' : height = atoi(&argv[i][1]);break;
            case 'f' : screenflags = screenflags | SDL_WINDOW_FULLSCREEN_DESKTOP;break;
        }
    }


    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

     // Create an application window with the following settings:
     window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        640,                               // width, in pixels
        480,                               // height, in pixels
        screenflags                        // flags - see below
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    int frame_counter = 0;
    while (!exit) {
        SDL_Event event;        
        SDL_Delay(100);
        frame_counter = (frame_counter++)%20;
        if (g_workspace.mode == PREVIEW && frame_counter==0) {
            for (int i = 1;i<10;i++){
                if (g_workspace.canvas_list[(g_workspace.active_canvas+i)%9] != NULL){
                    g_workspace.active_canvas = (g_workspace.active_canvas+i)%9;
                    break;
                }
            }
        }
        draw(window);
        
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE: exit=1;break;
                    case SDLK_LEFT: 
                    if ((g_workspace.cursor.x-g_workspace.start.x == 0) && (g_workspace.start.x>0)) g_workspace.start.x--;                 
                        g_workspace.cursor.x > 0 ? g_workspace.cursor.x--:0;
                        break;
                    case SDLK_RIGHT: 
                        g_workspace.cursor.x++; 
                        if ( (g_workspace.cursor.x+g_workspace.start.x)*g_workspace.scale >= SDL_GetWindowSurface(window)->w) g_workspace.start.x++;
                        break;
                    case SDLK_UP:
                        if ((g_workspace.cursor.y-g_workspace.start.y==0) && (g_workspace.start.y>0)) g_workspace.start.y--;                
                        g_workspace.cursor.y > 0 ? g_workspace.cursor.y--:0; 
                        break;
                    case SDLK_DOWN:  
                        g_workspace.cursor.y++;    
                        if ( (g_workspace.cursor.y+g_workspace.start.y)*g_workspace.scale > SDL_GetWindowSurface(window)->h) g_workspace.start.y++;                 
                        break;
                    case SDLK_SPACE: g_workspace.canvas_list[g_workspace.active_canvas]->pixels[g_workspace.cursor.x + g_workspace.cursor.y * g_workspace.canvas_list[g_workspace.active_canvas]->width] =
                        SDL_MapRGB( SDL_GetWindowSurface(window)->format, g_workspace.cursor_color.r, g_workspace.cursor_color.g, g_workspace.cursor_color.b ); break;
                    case SDLK_n: g_workspace.canvas_list[g_workspace.active_canvas]->pixels[g_workspace.cursor.x + g_workspace.cursor.y * g_workspace.canvas_list[g_workspace.active_canvas]->width] =
                        SDL_MapRGB( SDL_GetWindowSurface(window)->format, RGB_BLACK.r, RGB_BLACK.g, RGB_BLACK.b ); break;
                    case SDLK_g: g_workspace.show_grid = !g_workspace.show_grid; break;
                    case SDLK_z: g_workspace.scale = (g_workspace.scale << 1) & 255; if (g_workspace.scale==0) g_workspace.scale=1; break;
                    case SDLK_p: g_workspace.mode = PREVIEW;break;
                    case SDLK_e: g_workspace.mode = EDIT;break;      
                    case SDLK_c: g_workspace.cursor_color.r = g_workspace.cursor_color.r++%256;break;
                    case SDLK_v: g_workspace.cursor_color.g = g_workspace.cursor_color.g++%256;break;
                    case SDLK_b: g_workspace.cursor_color.b = g_workspace.cursor_color.b++%256;break;
                    default:
                        printf("Unhandled key : %d\n",event.key.keysym.sym);
                        int key = -1;
                        switch (event.key.keysym.scancode)
                        {
                            case SDL_SCANCODE_0: key = 0;break;
                            case SDL_SCANCODE_1: key = 1;break;
                            case SDL_SCANCODE_2: key = 2;break;
                            case SDL_SCANCODE_3: key = 3;break;
                            case SDL_SCANCODE_4: key = 4;break;
                            case SDL_SCANCODE_5: key = 5;break;
                            case SDL_SCANCODE_6: key = 6;break;
                            case SDL_SCANCODE_7: key = 7;break;
                            case SDL_SCANCODE_8: key = 8;break;
                            case SDL_SCANCODE_9: key = 9;break;      
                        }
                        if (key != -1) {
                            if (g_workspace.canvas_list[key] == NULL) g_workspace.canvas_list[key] = create_canvas(64,64);
                            g_workspace.active_canvas = key;
                            printf("ACTIVE CANVAS: %d\n",g_workspace.active_canvas);
                        }
                        break;

                }
                printf("cursor: %d %d scale: %d start: %d\n",g_workspace.cursor.x,g_workspace.cursor.y,g_workspace.scale,g_workspace.start.x);
                break;       
            }
        }
    }

    // Close and destroy the window
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

