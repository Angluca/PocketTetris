#ifndef __POCKET_TETRIS_H_140927__
#define __POCKET_TETRIS_H_140927__
/**
 * author: Angluca
 * email: to@angluca.com
 * date: 2014/09/27
 */

class PocketTetris
{
public:
	//constructor and destructor
	PocketTetris(void);
	~PocketTetris(void);

    void mouseDown(int button);
    void mouseMove(int x, int y);
	bool init();
	void update();
	void draw();
};

#endif /* __POCKET_TETRIS_H_140927__ */

