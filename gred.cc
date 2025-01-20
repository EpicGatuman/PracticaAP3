#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <map>
#include <climits>
#include <ctime>
#include<algorithm>

using namespace std;

typedef pair<int, int> P;
typedef vector<vector<bool>> Matrix;
typedef pair<P, int>    PI;
typedef vector<P>       VP;
typedef vector<PI>     VPI;
typedef pair<P, P>      PP;
typedef vector<PP>     VPP;

const PP UNDEF({-1, -1}, {-1, -1});

ifstream in;
clock_t start;
int  W, N;
int L = 0;
int C = 0;
double extra_time = 0;
VPI shapes;
VPP  sol;

void read_instance(const char* file) {
  /*This function reads the given file and extracts the data needed to execute the program
  */
  ifstream in(file);
  in >> W >> N;
  int n = N;
  int ni, pi, qi;
  while (n != 0) {
    in >> ni >> pi >> qi;
    n -= ni;
    C += max(pi,qi)*ni;
    shapes.push_back(make_pair(make_pair(pi,qi),ni));
  }
  //Pieces are ordered by area as it reduces execution time
  sort(shapes.begin(),shapes.end(), [](const auto &a, const auto &b){
    return (a.first.first*a.first.second)>(b.first.first*b.first.second);
  });
}


void write_instance(const char* file){
    /*This function creates or overwrites the given file with the current most optimal answer.
    The answer will be given in the following order, all separated into different lines.
    First the time it took to find the current answer.
    Second the length of the board.
    And the rest of the lines correspond to the upper-left and bottom-right coordinates of the placed pieces.
    */
    ofstream out(file);
    clock_t end = clock();
    double duration = double(end-start)/CLOCKS_PER_SEC;
    out.setf(ios::fixed);
    out.precision(3);
    out << duration << endl << L << endl;
    for( PP coords : sol){
        out << coords.first.first << ' ' << coords.first.second << ' ';
        out << coords.second.first << ' ' << coords.second.second << endl;
    }
    out.close();
}

bool inside(Matrix& graella,const P coord, const P shape){
    /*This function checks if the given piece can fit into the board.

    INPUTS: graella (Matrix of the board), coord (coordenate in which to place), shape (width and height of the piece to place)
    OUTPUT: bool (whether the piece is placeable or not)
    */
   //Check if out of bounds
    if(coord.second == -1 or coord.first == -1) return false;
    if(coord.second + shape.second > C) return false;
    if(coord.first + shape.first > W) return false;
    //Check if the spaces are already filled
    for(int i=0; i<shape.first; ++i){
        for(int j=0; j<shape.second; ++j){
            if(graella[coord.first+i][coord.second+j]) return false;
        }
    }
    return true;
}

void editarMat(Matrix& graella, const P coord, const P shape, const bool entry){
    /*This function updates the board to showcase where a new piece as been added or deleted.

    INPUT: graella (Matrix of the board), coord (coordinate where the upper-left corner of the piece is to be placed),
    shape (width and height of the given piece), entry (bool to showcase placement or delition).
    */
    for(int i=0; i<shape.first; ++i){
        for(int j=0; j<shape.second; ++j){
            graella[coord.first+i][coord.second+j] = entry;
        }
    }
}

void addtosol(P coord, P shape, const bool make){
    /*Adds or deletes the coordinates of the placed piece (upper-left and bottom-right) to the set of placed pieces.
    INPUT: coord (coordenate where the upper-left corner of the piece is placed), shape (width and height of the piece),
    make (bool that register whether the piece has to be added or deleted)
    */
    if (make){
        sol.push_back(make_pair(coord,make_pair(coord.first + shape.first-1,coord.second+shape.second-1))); 
    }
    else sol.pop_back();

}

P coordenada(Matrix& graella, P coord, int x){
    /*Finds the next unfilled coordenate.
    INPUT: graella (Matrix of the board), coord (pair of coordenates), 
    x (int that showcases a shift towards the right of the board)
    OUTPUT: pair of the next available coordenates
    */
    coord.first += x;
    if (coord.first == W) {coord.first = 0;++ coord.second;}
    while (graella[coord.first][coord.second]){
        ++ coord.first;
        if(coord.first == W){coord.first = 0;++ coord.second;}
    }
    return coord;
}

void greedy(){
    Matrix graella (W,vector<bool>(C,false));
    P coord = make_pair(0,0);
    while(N!=0){
        bool placed = false;
        for(PI& shape : shapes){
            if(shape.second!=0){
                int maxim = max(shape.first.first,shape.first.second);
                int minim = min(shape.first.first,shape.first.second);
                if(inside(graella,coord,make_pair(maxim,minim))){
                    editarMat(graella,coord,make_pair(maxim,minim),true);
                    addtosol(coord,make_pair(maxim,minim),true);
                    L = max(L,coord.second+minim);
                    coord = coordenada(graella,coord,maxim);
                    --shape.second;
                    --N;
                    placed = true;
                    break;
                }
                else if(inside(graella, coord, make_pair(minim,maxim))){
                    editarMat(graella,coord,make_pair(minim,maxim),true);
                    addtosol(coord,make_pair(minim,maxim),true);
                    L = max(L,coord.second+maxim);
                    coord = coordenada(graella,coord,minim);
                    --shape.second;
                    placed = true;
                    --N;
                    break;
                }
            }
        }
        if(not placed){
            coord = coordenada(graella,coord,1);
        }
    }
}

int main(int argc, char** argv) {
  start = clock();
  // Write help message.
  if (argc == 1) {
    cout << "Makes a sanity check of a solution" << endl;
    cout << "Usage: " << argv[0] << " INPUT_FILE OUTPUT_FILE" << endl;
    exit(0);
  }
  assert(argc == 3);

  read_instance(argv[1]);

  greedy();

  write_instance(argv[2]);
}