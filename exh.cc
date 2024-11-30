#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <map>
#include <climits>

using namespace std;

typedef pair<int, int> P;
typedef vector<vector<bool>> Matrix;
typedef pair<P, int>    PI;
typedef vector<PI>     VPI;
typedef pair<P, P>      PP;
typedef vector<P>       VP;
typedef vector<PP>     VPP;
typedef vector<VPP>   VVPP;

const PP UNDEF({-1, -1}, {-1, -1});

ifstream in;
int  W, N;
int L = INT_MAX;
int C = 0;
VPI shapes;
VPP  sol;
VVPP board;

void read_instance(const char* file) {
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
}

//Fa servir solucio parcial en la variable global per escriure-la
//W i L tambe son variables globals
void write_instance(const char* file){
    ofstream out(file);
    out << L << endl;
    for( PP coords : sol){
        out << coords.first.first << ' ' << coords.first.second << ' ';
        out << coords.second.first << ' ' << coords.second.second << endl;
    }
    out.close();
}

bool inside(Matrix& graella,const P coord, const P shape){
    if(coord.first + shape.first > W) return false;
    for(int i=0; i<shape.first; ++i){
        for(int j=0; j<shape.second; ++j){
            if(graella[coord.first+i][coord.second+j]) return false;
        }
    }
    return true;
}

void editarMat(Matrix& graella, const P coord, const P shape, const bool entry){
    for(int i=0; i<shape.first; ++i){
        for(int j=0; j<shape.second; ++j){
            graella[coord.first+i][coord.second+j] = entry;
        }
    }
}

void addtosol(P coord, P shape, const bool make){
    if (make){
        sol.push_back(make_pair(coord,make_pair(coord.first + shape.first-1,coord.second+shape.second-1))); 
    }
    else sol.pop_back();

}

P coordenada(Matrix& graella, P coord, int x){
    coord.first += x;
    if (coord.first == W) {coord.first = 0;++ coord.second;}
    while (graella[coord.first][coord.second]){
        ++ coord.first;
        if(coord.first == W){coord.first = 0;++ coord.second;}
    }
    return coord;
    
}

//En graella, el primer argument es coordenada incial i final del objecte
//Solucio parcial corre per global
void exhaustive(const char* file, Matrix& graella, P coord, int n, int l_par){
    if(l_par>=L) return;
    int k = 0;
    for(PI m: shapes){
        k += (m.first.first*m.first.second)*m.second;
    }
    if(k/W + coord.second >= L) return;
    if(n==N){
        L = l_par;
        cout << "ans found " << L << endl;
        write_instance(file);
        return;
    }
    else{
        for(int i=0;i<int(shapes.size());++i){
            if(shapes[i].second != 0){
                int minim = min(shapes[i].first.first,shapes[i].first.second);
                int maxim = max(shapes[i].first.first,shapes[i].first.second);
                //Mirem si min-max cap
                if(inside(graella,coord,make_pair(maxim,minim))){
                    -- shapes[i].second;
                    editarMat(graella,coord,make_pair(maxim,minim),true);
                    P newcoord = coordenada(graella, coord,maxim);
                    int newl_par = max(l_par, coord.second + minim);
                    addtosol(coord, make_pair(maxim,minim), true);
                    exhaustive(file,graella, newcoord, n+1, newl_par);
                    ++ shapes[i].second;
                    editarMat(graella,coord,make_pair(maxim,minim),false);
                    addtosol(coord, make_pair(minim,maxim),false);
                }
                //Mirem si max-min cap
                else if(minim!= maxim and inside(graella,coord,make_pair(minim,maxim))){
                    -- shapes[i].second;
                    editarMat(graella,coord,make_pair(minim,maxim),true);
                    P newcoord = coordenada(graella, coord,minim);
                    int newl_par = max(l_par, coord.second + maxim);
                    addtosol(coord, make_pair(minim,maxim), true);
                    exhaustive(file,graella, newcoord, n+1, newl_par);
                    ++ shapes[i].second;
                    editarMat(graella,coord,make_pair(minim,maxim),false);
                    addtosol(coord, make_pair(minim,maxim), false);
                }
            }
        }
       if(coord.second + 1 <= l_par and coord.second != 0) exhaustive(file,graella,coordenada(graella, coord,1), n, l_par);
    }
}

int main(int argc, char** argv) {

  // Write help message.
  if (argc == 1) {
    cout << "Makes a sanity check of a solution" << endl;
    cout << "Usage: " << argv[0] << " INPUT_FILE OUTPUT_FILE" << endl;
    exit(0);
  }
  assert(argc == 3);
  read_instance(argv[1]);
  Matrix graella (W,vector<bool>(C,false));
  exhaustive(argv[2], graella, make_pair(0,0),0,0);
}