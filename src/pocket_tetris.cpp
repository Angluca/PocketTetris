/**
 * author: Angluca
 * email: to@angluca.com
 * date: 2014/09/27
 */
#include "pocket_tetris.h"
#include <stdint.h>
#include <stdlib.h>
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
enum {
    GAME_MENU = 0
    ,GAME_RUN
    ,GAME_OVER
};
static int s_game_state = GAME_MENU;

static const int s_width = 16;
static const int s_height = 24;
static const int s_total = s_width * s_height;

static float s_grid_width = 10;
static float s_grid_height = 10;
static int s_mouse_x, s_mouse_y;
static int s_mouse_btn;

static int s_score;

struct Shape {
	uint64_t shapes;
    uint8_t idx;
    uint16_t shape;
	int x, y;
    bool is_drop;
} s_cur_shape;

static void init_shape(struct Shape *p, int x = s_width>>1, int y=0) {
    memset(p, 0, sizeof(Shape));
    p->x = x - 2;
    p->y = y;
}

static const uint64_t s_shapes[]= {
	4919056758565240847
    ,28710885717835878
    
    ,172263536190554310
    ,315815387799355500
    
    ,343400450913206350
    
    ,441916709939839022
    ,154812207958589582

};
static bool s_scene[s_total];

static inline uint16_t _get_shape(uint64_t shape, int idx) {
    uint16_t ret = ((shape >> (idx*16)) & 0xffff);
    return ret;
//    return (
//            (((ret&0xf00) >> 4))
//            |(((ret&0xf0) << 4))
//            |(((ret&0xf) << 12))
//            |(((ret&0xf000) >> 12))
//            );
}
static bool _clamp_pos(uint16_t shape, int &sx, int &sy, int mw, int mh) {
    uint16_t row1 = shape & 34952;
    uint16_t row2 = shape & 17476;
    uint16_t row3 = shape & 8738;
    uint16_t row4 = shape & 4369;

    uint16_t col1 = shape & 15;
    uint16_t col2 = shape & 240;
    uint16_t col3 = shape & 3840;
    uint16_t col4 = shape & 61440;
    int lidx = (row4?0: ( row3?-1:( row2?-2:-3 )));
    int ridx = (row1?-4:( row2?-3:( row3?-2:-1 )));
    sx = (sx < lidx ? lidx
          : ((sx - ridx ) > mw ? mw + ridx:sx));

    int tidx = (col1?0: ( col2?-1:( col3?-2:-3 )));
    int bidx = (col4?-4:( col3?-3:( col2?-2:-1 )));

    bool ret = (sy - bidx) > mh;
    sy = (sy < tidx ? tidx
          : ((sy - bidx) > mh ? mh + bidx:sy));
    return ret;
}

static uint16_t _is_collision(bool *map, uint16_t shape, int sx, int sy, int mw, int mh) {
    _clamp_pos(shape, sx, sy, mw, mh);
    uint64_t data = 0;
    for (int y = 0; y < 4; ++y) {
        for(int x=0; x < 4; ++x) {
            data |= (map[(sy + y)* mw + (sx + x)] << (y*4+x));
        }
    }
    return data & shape;
}

static bool _is_edge(uint16_t shape, int sx, int sy, int mw, int mh) {
    return _clamp_pos(shape, sx, sy, mw, mh);
}

static void _shape2scene(bool *map, uint16_t shape, int sx, int sy, int mw, int mh) {
    _clamp_pos(shape, sx, sy, mw, mh);
    int w = s_width - sx, h = s_height - sy;
    w = w<4?w:4;
    h = h<4?h:4;
    for (int y = 0; y < h; ++y) {
        for(int x=0; x < w; ++x) {
            map[(sy + y)* mw + (sx + x)] |= ((shape & (1<<(y*4 + x)))?true:false);
//            std::cout << (map[(sy + y)* mw + (sx + x)]?"*":"0");
        }
//        std::cout << std::endl;
    }
}
static bool _is_full(bool *map, int sy, int mw) {
	bool ret = true;
	for(int i=0; i<mw; ++i) {
		if(!map[sy*mw+i]) {
			ret = false;
			break;
		}
	}
	return ret;
}
static bool _is_connect(bool *map, int sy) {
	if(sy == 0) return false;

	for(int i=0; i<s_width; ++i) {
		if(map[(sy - 1)* s_width + i]) return true;
	}
	return false;
}
static int _get_last_connect(bool *map, int sy) {
	while(sy > 0) {
		if(!_is_connect(map, sy)) break;
        --sy;
	}
	return sy;
}

static void _map_print(bool *map, int w, int h);

static int _clear_full(bool *map, int sy, int mw, int num = 4) {
	int ret = 0, last_connect = 0;
	bool is_full = false;
	for(int i=0; i<num; ++i) {
		is_full = _is_full(map, sy+i, mw);
		if(is_full) {
			++ret;
			last_connect = _get_last_connect(map, sy+i-1);
            memset(map+(sy+i)*mw, 0 , mw);
//            _map_print(map+(last_connect) * mw, s_width, sy+i-last_connect);
            memcpy(map+(last_connect+1)*mw, map+(last_connect)*mw , (sy+i - last_connect)*mw);
            memset(map+(last_connect)*mw, 0 , mw);
		}
	}
	return ret;
}

static void _shape_print(uint16_t shape){
    std::cout << "shape=" << shape << std::endl;
    for(int i=0; i<4; ++i) {
        for(int j=0; j<4; ++j) {
			std::cout << ((shape & (1<<(i*4+j)))?"*":"0");
        }
        std::cout << std::endl;
    }
}

static void _map_print(bool *map, int w, int h){
    for(int i=0; i<h; ++i) {
        for(int j=0; j<w; ++j) {
            std::cout << (map[i*w+j]?"*":"0");
        }
        std::cout << std::endl;
    }
}


static void _draw_cur_shape(bool *map, uint16_t shape, int sx, int sy, int mw, int mh, int gw, int gh) {
    _clamp_pos(shape, sx, sy, mw, mh);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    gl::color( Color(0.8f, 0.8f, 1.0f));
    int tx, ty;
    for (int y = 0; y < 4; ++y) {
        for(int x=0; x < 4; ++x) {
            if( (shape & (1<<(y*4 + x))) ) {
                tx = (sx + x) * gw;
                ty = (sy + y) * gh;
                Vec2i pos(tx,ty);
                gl::drawSolidRect(Rectf(pos, pos +Vec2i(gw, gh)));
            }
        }
    }
    glPopAttrib();
}

static void _draw_scene(bool *map, int w, int h, int gw, int gh) {
    int idx, x, y;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    for(int col=0; col<h; ++col) {
        for(int row=0; row<w; ++row) {
            idx = col * w + row;
            if(map[idx]) {
                x = row * gw;
                y = col * gh;

                Vec2i pos(x,y);
                gl::color( Color(1.0f, 0.8f, 1.0f));
                gl::drawSolidRect(Rectf(pos, pos +Vec2i(gw, gh)));
                gl::color( Color(.6f, 0.6f, .6f));
                gl::drawStrokedRect(Rectf(pos, pos +Vec2i(gw, gh)));
            }
        }
    }
}

static void _draw_score(int x, int y) {
//    float n = s_score - s_score_anim;
//    if(n > 0) {
//        s_score_anim += 0.2f;
//    } else {
//        s_score_anim = s_score;
//    }

    Color color(1.0f, 1.0f ,1.0f);

//    if(n) color = Color(0.8f, 1.0f, 1.0f);
    gl::drawStringCentered(std::to_string((int)s_score), Vec2i(x,y), color);
}

static void _get_rand_shape(uint64_t &shapes, uint16_t &shape, uint8_t &idx) {
    shapes = s_shapes[rand() % (sizeof(s_shapes)/sizeof(uint64_t))];
    idx = rand() % 4;
    shape = _get_shape(shapes , idx);
}
static void _make_grid_size() {
    s_grid_width = getWindowWidth() / s_width;
    s_grid_height = getWindowHeight() / s_height;
}
static bool _make_cur_shape_pos(int x, int y, bool is_match = false) {
    bool ret = false;
    int row = x/s_grid_width;
    int col = y/s_grid_height;
    int vx = row - s_cur_shape.x;
    int vy = col - s_cur_shape.y;
    vx = (vx<0?-1:(vx>0?1:0));
    vy = (vy<0?-1:(vy>0?1:0));
    if((vx || vy) && !_is_collision(s_scene, s_cur_shape.shape, s_cur_shape.x + vx, s_cur_shape.y + vy, s_width, s_height))
    {
        s_cur_shape.x += vx;
        s_cur_shape.y += vy;
        ret = true;
    } else if(!is_match){
        if((vx || vy) && !_is_collision(s_scene, s_cur_shape.shape, s_cur_shape.x + vx, s_cur_shape.y, s_width, s_height)) {
            s_cur_shape.x += vx;
            ret = true;
        } else if((vx || vy) && !_is_collision(s_scene, s_cur_shape.shape, s_cur_shape.x, s_cur_shape.y + vy, s_width, s_height)) {

            s_cur_shape.y += vy;
            ret = true;
        }
    }
    return ret;
}

static bool _game_init() {
    memset(s_scene, 0, s_total);
    init_shape(&s_cur_shape);
    s_score = 0;
    
    srand(time(NULL));
    int n = rand() % 10;
    for(int i=0; i<n; ++i) {
        _get_rand_shape(s_cur_shape.shapes, s_cur_shape.shape, s_cur_shape.idx);
        /* test */
        uint16_t shape = s_cur_shape.shape;
        _shape2scene(s_scene, shape, rand() %s_width, rand() % (s_height-4) + 4, s_width, s_height);
        //    _map_print(s_scene, s_width, s_height);
    }
    
    _get_rand_shape(s_cur_shape.shapes, s_cur_shape.shape, s_cur_shape.idx);
    return true;
}
static void _shape_rotate()
{
    if(s_cur_shape.is_drop) return;

    uint8_t idx = (s_cur_shape.idx+1) % 4;
    uint16_t shape = _get_shape(s_cur_shape.shapes , idx);
    if(!_is_collision(s_scene, shape, s_cur_shape.x, s_cur_shape.y, s_width, s_height)) {
        s_cur_shape.idx = idx;
        s_cur_shape.shape = shape;
    }
}

static void _update_keys() {
    switch (s_game_state) {
        case GAME_MENU:
        {
            if(s_mouse_btn) {
                s_game_state = GAME_RUN;
                srand(time(NULL));
                _game_init();
            }
        }
            break;
        case GAME_RUN:
        {
            if(s_mouse_btn & 2) {
                //_get_rand_shape(s_cur_shape.shapes, s_cur_shape.shape, s_cur_shape.idx);
                s_cur_shape.is_drop = true;
            } else if(s_mouse_btn & 1) {
                _shape_rotate();
            }
        }
            break;
        case GAME_OVER:
        {
            if(s_mouse_btn) {
                s_game_state = GAME_RUN;
                srand(time(NULL));
                _game_init();
            }
        }
            break;
        default:
            break;
    }
    s_mouse_btn = 0;
}

static void _update_logic() {
    switch (s_game_state) {
        case GAME_MENU:
        {
            
        }
            break;
        case GAME_RUN:
        {
            if(!s_cur_shape.is_drop) {
                int x = s_mouse_x, y = s_mouse_y;
                Vec2f center = getWindowCenter();
                int vx = (x - center.x) / 5;
                int vy = (y - center.y) / 5;
                _make_cur_shape_pos(x-2*s_grid_width + vx, y + vy);
            } else {
                int x = s_cur_shape.x;
                int y = s_cur_shape.y;
                
                if(!_make_cur_shape_pos(x * s_grid_width, (y+1)*s_grid_height, true) ||
                   _is_edge(s_cur_shape.shape, s_cur_shape.x, s_cur_shape.y, s_width, s_height)) {
                    
                    _clamp_pos(s_cur_shape.shape, x, y, s_width, s_height);
                    
                    _shape2scene(s_scene, s_cur_shape.shape, x, y, s_width, s_height);
                    int num = _clear_full(s_scene, y, s_width);
                    //			std::cout << num << std::endl;
                    if(num > 0) {
                        s_score += (num * num);
                    }
                    
                    init_shape(&s_cur_shape);
                    _get_rand_shape(s_cur_shape.shapes, s_cur_shape.shape, s_cur_shape.idx);
                    if(_is_collision(s_scene, s_cur_shape.shape, s_cur_shape.x, s_cur_shape.y, s_width, s_height)) {
                        s_game_state = GAME_OVER;
                    }
                }
            }
        }
            break;
        case GAME_OVER:
        {
            
        }
            break;
        default:
            break;
    }

}

//constructor
PocketTetris::PocketTetris(void)
{
}

//destructor
PocketTetris::~PocketTetris(void)
{
};

bool PocketTetris::init()
{

	return _game_init();
}

void PocketTetris::mouseDown(int button)
{
    s_mouse_btn = button;

}
void PocketTetris::mouseMove(int x, int y)
{
    s_mouse_x = x;
    s_mouse_y = y;
}

void PocketTetris::update()
{
    _make_grid_size();
    
    _update_keys();
    _update_logic();
//    
//    static int loop = 0;
//    if(++loop > 900) {
//        srand(time(NULL));
//    }
}

void PocketTetris::draw()
{
    switch (s_game_state) {
        case GAME_MENU:
        {
            gl::drawStringCentered("点击任意键开始游戏", getWindowCenter());
        }
            break;
        case GAME_RUN:
        {
            _draw_scene(s_scene, s_width, s_height, s_grid_width, s_grid_height);
            _draw_cur_shape(s_scene, s_cur_shape.shape, s_cur_shape.x, s_cur_shape.y, s_width, s_height, s_grid_width, s_grid_height);
            _draw_score(getWindowWidth() >> 1, 10);
        }
            break;
        case GAME_OVER:
        {
            _draw_scene(s_scene, s_width, s_height, s_grid_width, s_grid_height);
            _draw_cur_shape(s_scene, s_cur_shape.shape, s_cur_shape.x, s_cur_shape.y, s_width, s_height, s_grid_width, s_grid_height);
            _draw_score(getWindowWidth() >> 1, 10);
            gl::drawStringCentered(std::string("你的分数是: ")+std::to_string((int)s_score), getWindowCenter() - Vec2i(0,20));
            gl::drawStringCentered("\n点击任意键开始游戏", getWindowCenter());
        }
            break;
        default:
            break;
    }

}
