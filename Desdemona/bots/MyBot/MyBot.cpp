/*
* @file botTemplate.cpp
* @author Arun Tejasvi Chaganty <arunchaganty@gmail.com>
* @date 2010-02-04
* Template for users to create their own bots
*/

#include "Othello.h"
#include "OthelloBoard.h"
#include "OthelloPlayer.h"
#include <cstdlib>
#include <ctime>
#include <map>
#include <string>
using namespace std;
using namespace Desdemona;

class MovesCache
{
    public:
        MovesCache()
        {
        }

        void add(string id, Move value)
        {
            bins[id] = make_pair(value.x, value.y);
        }

        Move get(string id, bool& found)
        {
            Move ret_value = Move::empty();
            if ( bins.find(id) != bins.end()) 
	    {
		    pair<int,int> x = bins[id];
		    Move m(x.first, x.second);
		    ret_value = m; 
		    found=true; 
	    }
            else { found=false; }
            return ret_value;
        }
    private:
        map< string, pair<int,int> > bins;
};

class MyBot: public OthelloPlayer
{
    public:
        /**
         * Initialisation routines here
         * This could do anything from open up a cache of "best moves" to
         * spawning a background processing thread. 
         */
        MyBot( Turn turn );

        /**
         * Play something 
         */
        virtual Move play( const OthelloBoard& board );
    private:
	Coin player;
	Coin opponent;
        int searchDepth;
        int actions;
	MovesCache cache;

        Move alphaBetaStart( const OthelloBoard& board, int depth, Coin player );
        float alphaBetaValue( const OthelloBoard& board, int depth, Coin player, float alpha, float beta );

	float getEval(const OthelloBoard& board, Coin player, Coin opponent);
	float getUtil(const OthelloBoard& board, Coin player, Coin opponent);
	void getScoreOfBoard(const OthelloBoard& board, float& playerScore, float& opponentScore, Coin player, Coin opponent);
        string getBoardId( const OthelloBoard& board );
	void UpdateCountsCornerNeighbours(const OthelloBoard& board, Coin player, Coin opponent, int& pcount, int& ocount, int a, int b, int a1, int b1, int a2, int b2, int a3, int b3 );

	float getPFD(const OthelloBoard& board, Coin player, Coin opponent);
	float getMobility(const OthelloBoard& board, Coin player, Coin opponent);
	float cornersHeuristic(const OthelloBoard& board, Coin player, Coin opponent);
	float cornerNeighbourHeuristic(const OthelloBoard& board, Coin player, Coin opponent);
};

Move MyBot::alphaBetaStart( const OthelloBoard& board, int depth, Coin player )
{
	this->actions++;
	cout << "actions = " << actions << endl;

	if (actions==25)
	{
		cout << "25 actions reached " << endl;
		this->searchDepth = 15;
	}
	float bestValue = -1000000.0;
	Move bestMove = Move::empty();

	Coin opponent;
	if (player == this->player) opponent = this->opponent;
	else opponent = this->player;
    	list<Move> moves = board.getValidMoves( player );
    	list<Move>::iterator it = moves.begin();
	for( ; it != moves.end(); ++it)
	{
		OthelloBoard copyB(board);
		copyB.makeMove( player, *it );

		float alpha = -99999;
		float beta  =  99999;
		float val = -1*this->alphaBetaValue(copyB, depth, opponent, alpha, beta);

		if (val == 100000) return *it;
		if (val > bestValue)
		{
			bestValue = val;
			bestMove = *it;
		}
	}
	return bestMove;
}

float MyBot::alphaBetaValue( const OthelloBoard& board, int depth, Coin player, float alpha, float beta )
{
	int fix;
	Coin opponent;
	if (player == this->player) { opponent = this->opponent; fix=1;}
	else { opponent = this->player; fix=-1; }

	float bestValue = -100000;
	if (depth == 0)
	{
		return this->getEval(board, player, opponent) * fix;
	}


    	list<Move> moves = board.getValidMoves( player );
	if (moves.size() == 0)
	{
		if ( board.getValidMoves(opponent).size() == 0 )
			return getUtil(board, player, opponent) * fix;

		OthelloBoard copyB(board);
		return -1*this->alphaBetaValue(copyB, depth-1, opponent, -beta,-alpha);
	}

    	list<Move>::iterator it = moves.begin();
	for( ; it != moves.end(); ++it)
	{
		OthelloBoard copyB(board);
		copyB.makeMove( player, *it );

		float curValue = -1*this->alphaBetaValue(copyB, depth-1, opponent, -beta,-alpha);

		if (bestValue < curValue)
		{
			bestValue = curValue;
			if (bestValue == 100000)
			{
				this->cache.add(this->getBoardId(copyB), *it);
			}
		}

		alpha = max(alpha, bestValue);
		if (alpha >= beta) break;
	}
	return bestValue;
}

float MyBot::getEval(const OthelloBoard& board, Coin player, Coin opponent)
{
	float mobility = this->getMobility(board, player, opponent);
	float corner = this->cornersHeuristic(board, player, opponent);
	float cornerNeighbours = this->cornerNeighbourHeuristic(board, player, opponent);
	float pfd = this->getPFD(board, player, opponent);
	return (801.724*corner) + (12.5*324.026*cornerNeighbours) + (78.922*mobility) + pfd;
}

float MyBot::getPFD(const OthelloBoard& board, Coin player, Coin opponent)
{
	int B =8;
	int x[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	int y[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	int v[8][8] = 
	{
		{20, -3, 11, 8, 8, 11, -3, 20},
		{-3, -7, -4, 1, 1, -4, -7, -3},
		{11, -4, 2, 2, 2, 2, -4, 11},
		{8, 1, 2, -3, -3, 2, 1, 8},
		{8, 1, 2, -3, -3, 2, 1, 8},
		{11, -4, 2, 2, 2, 2, -4, 11},
		{-3, -7, -4, 1, 1, -4, -7, -3},
		{20, -3, 11, 8, 8, 11, -3, 20}
	};

	int d = 0;
	int pt = 0;
	int ot = 0;
	int pft = 0;
	int oft = 0;
	for( int i = 0; i<B; i++ )
    	{
        for( int j = 0; j<B; j++ )
        {
	    Coin val = board.get(i,j);
	    if (val == player) 
	    {
		    d += v[i][j];
		    pt++;
	    }
	    else if (val == opponent)
	    {
		    d -= v[i][j];
		    ot++;
	    }
	    else
	    {
		    for (int k = 0; k<B; k++)
		    {
			    int x1 = i + x[k];
			    int y1 = j + y[k];
			    int l = B-1;
			    if (x1>=0 && y1>=0 && x1<=l && y1<=l && board.get(x1, y1)==EMPTY)
			    {
				    if (val == player) pft++;
				    else 
				    { 
					    oft++;
					    break;
				    }
			    }
		    }
	    }
        }
   	}

	float p = 0.0;
	if (pt > ot) p = (100.0*pt) / (pt+ot);
	else if (pt < ot) p = -(100.0*ot) / (pt+ot);

	float f = 0.0;
	if (pft > oft) f = -(100.0 * pft) / (pft+oft);
	else if (pft < oft) f = (100.0 * oft) / (pft+oft);
	return 10*(d+p) + (74.396*f);
}

float MyBot::getMobility(const OthelloBoard& board, Coin player, Coin opponent)
{
	float m = 0;
	float pm = board.getValidMoves( player ).size();
	float om = board.getValidMoves( opponent ).size();

	if(pm > om) m = (100.0 * pm) / (pm + om);
	else if (om > pm)m = -(100.0 * om) / (pm + om);

	if (pm!=0 || om!=0) m = 100.0*(pm-om) / (pm+om);
	return m;
}

bool isCorner(int x, int y, int B)
{
	int l = B-1;
	return (x==0&&y==0) || (x==l&&y==0) || (x==0&&y==l) || (x==l&&y==l);
}
float MyBot::cornersHeuristic(const OthelloBoard& board, Coin player, Coin opponent)
{
    int pcorners = 0;
    int ocorners = 0;
    int B = 8; // OthelloBoard::BOARD_SIZE
    for( int i = 0; i<B; i++ )
    {
        for( int j = 0; j<B; j++ )
        {
		if (isCorner(i,j, B))
		{
		    Coin val = board.get(i,j);
		    if (val == player) pcorners++;
		    else if (val == opponent) ocorners++;
		}
        }
    }
    return (pcorners - ocorners);
}

void MyBot::UpdateCountsCornerNeighbours(const OthelloBoard& board, Coin player, Coin opponent, int& pcount, int& ocount, int a, int b, int a1, int b1, int a2, int b2, int a3, int b3 )
{
	if (board.get(a,b) == EMPTY)
	{
		if (board.get(a1,b1) == player) pcount++;
		else if (board.get(a1,b1) == opponent) ocount++;

		if (board.get(a2,b2) == player) pcount++;
		else if (board.get(a2,b2) == opponent) ocount++;

		if (board.get(a3,b3) == player) pcount++;
		else if (board.get(a3,b3) == opponent) ocount++;
	}
}

float MyBot::cornerNeighbourHeuristic(const OthelloBoard& board, Coin player, Coin opponent)
{
	int pcount = 0;
	int ocount = 0;

	this->UpdateCountsCornerNeighbours(board, player, opponent, pcount, ocount, 
		0, 0, 0, 1, 1, 1, 1, 0);
	this->UpdateCountsCornerNeighbours(board, player, opponent, pcount, ocount, 
		0, 7, 0, 6, 1, 6, 1, 7);
	this->UpdateCountsCornerNeighbours(board, player, opponent, pcount, ocount, 
		7, 0, 7, 1, 6, 1, 6, 0);
	this->UpdateCountsCornerNeighbours(board, player, opponent, pcount, ocount, 
		7, 7, 6, 7, 6, 6, 7, 6);
	float l = ocount - 10 * pcount;
	return l;
}

float MyBot::getUtil(const OthelloBoard& board, Coin player, Coin opponent)
{
	float playerScore;
	float opponentScore;
	this->getScoreOfBoard(board, playerScore, opponentScore, player, opponent);
	if (playerScore > opponentScore) return 100000;
	if (playerScore == opponentScore) return 0;
	return -100000;
}

void MyBot::getScoreOfBoard(const OthelloBoard& board, float& playerScore, float& opponentScore, Coin player, Coin opponent)
{
    playerScore = 0;
    opponentScore = 0;
    int B = 8; // OthelloBoard::BOARD_SIZE
    for( int i = 0; i<B; i++ )
    {
        for( int j = 0; j<B; j++ )
        {
	    Coin val = board.get(i,j);
	    if (val == player) playerScore += 1;
	    else if (val == opponent) opponentScore += 1;
        }
    }
}

string MyBot::getBoardId(const OthelloBoard& board)
{
    int B = 8; // OthelloBoard::BOARD_SIZE
    string id = "1";
    for( int i = 0; i<B; i++ )
    {
        for( int j = 0; j<B; j++ )
        {
	    Coin val = board.get(i,j);
	    if (val == this->player) id += "1";
	    else if (val == this->opponent) id += "2";
	    else id += "0"; 
        }
    }
    return id;
}

MyBot::MyBot( Turn turn )
    : OthelloPlayer( turn )
{
    time_t t;
    time( &t );
    srand( t );
    this->searchDepth = 4;
    this->actions = 0;
    this->player = turn;
    this->opponent = turn == BLACK ? RED : BLACK ;
}

Move MyBot::play( const OthelloBoard& board )
{
    list<Move> moves = board.getValidMoves( turn );
    int randNo = rand() % moves.size();
    list<Move>::iterator it = moves.begin();

    string id = this->getBoardId(board);
    //cout << "board id " << id << endl;

    bool found = false;
    Move move = this->cache.get(id, found);
    if (found)
    {
	return move;
    }

    Move bestMove = this->alphaBetaStart(board, this->searchDepth, this->turn);    return bestMove;

    for(int i=0; i < randNo; it++, i++);
    return *it;
}

// The following lines are _very_ important to create a bot module for Desdemona

extern "C" {
    OthelloPlayer* createBot( Turn turn )
    {
        return new MyBot( turn );
    }

    void destroyBot( OthelloPlayer* bot )
    {
        delete bot;
    }
}


