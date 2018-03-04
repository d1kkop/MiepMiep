#include "Game.h"
#include "Network.h"
using namespace MyGame;


int main(int argc, char** argv)
{
	Game* g = new Game;
	g->init();
	g->run();
	delete g;

	Network::printMemoryLeaks();
}