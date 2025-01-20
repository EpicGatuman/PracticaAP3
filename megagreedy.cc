#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <map>
#include <climits>
#include <ctime>
#include<algorithm>

using namespace std;

struct surface{
    int level;
    int left;
    int right;
};

typedef pair<int, int> P;
typedef vector<surface> VS;
typedef pair<P, int>    PI;
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
    First the time it took to find the current answer. Second the length of the board.
    And the rest of the lines correspond to the upper-left and bottom-right coordinates of the 
    placed pieces.
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

void create_sup(surface& suf, int level, int left, int right){
    /*This class repressents surfaces upon which rectangles can be placed.
    Left and right showcase the horizontal bounds of the surface.
    Level showcases the height at which the surfaces lies.
    INPUT: suf (surface in question), level (the height), 
    left and right (horizontal coordinates)
    */
    suf.level = level;
    suf.left = left;
    suf.right = right;
}

void addtosol(const P& coord,const P& shape){
    /*Adds or deletes the coordinates of the placed piece (upper-left and bottom-right) to the set of placed pieces.
    INPUT: coord (coordenate where the upper-left corner of the piece is placed), shape (width and height of the piece),
    make (bool that register whether the piece has to be added or deleted)
    */
    sol.push_back(make_pair(coord,make_pair(coord.first + shape.first-1,coord.second+shape.second-1)));
}

P optimal_shape(int width){
    /*Finds the optimal piece, using the max(widht + 0.5*height) criterium, to be placed unto the surface
    INPUT: width of the surface
    OUTPUT: pair<int,int> that signify:
        1) The first int showcases the amount of times a piece has been placed and its sign whether it has
        been rotated or not (negative implies rotation)
        2) Index in shapes that identifies the piece being placed
    */
    P chosen = make_pair(0,-1);
    double maxim = 0;
    for(int j=0;j<int(shapes.size());++j){
        PI type = shapes[j];
        P shape = type.first;
        for(int i=1;i<=type.second;++i){
            //orientation 1
            if(shape.first*i<=width){
                if(shape.first*i + 0.5*shape.second > maxim){
                    maxim = shape.first*i + 0.5*shape.second;
                    chosen = make_pair(i,j);
                }
            }
            //orientation 2
            if(shape.second*i<=width){
                if(shape.second*i + 0.5*shape.first > maxim){
                    maxim = shape.second*i + 0.5*shape.first;
                    chosen = make_pair(-i,j);
                }
            }
        }
    }
    return chosen;      
}

P neighbours(VS& surfaces, surface srf){
    /*Pair of left and right indexes of the position of the neighbouring surfaces 
    (those next to it in the horizontal axis) in surfaces vector.
    INPUT: surfaces (vector of surfaces), srf (surface whose neighbour it finds)
    OUTPUT: pair<int,int> of the left and right indexes of the neigbouring surfaces in the surfaces vector
    */
    int left = -1;  // Index of the left neighbor
    int right = -1; // Index of the right neighbor
    for (size_t i = 0; i < surfaces.size(); ++i) {
        if (surfaces[i].right == srf.left) {
            left = i;
        }
        if (surfaces[i].left == srf.right) {
            right = i;
        }
    }

    return make_pair(left,right);  
}

void add_surface(VS& surfaces, const surface& nw){
    /*Adds a new surface into the surfaces vector such that it stays sorted (sorted according to level and then by width)
    INPUT: surfaces (vector of surfaces), nw (new surface that needs to be placed into the vector)
    */
    auto compare = [](const surface& a, const surface& b) {
        if (a.level != b.level) {
            return a.level < b.level;
        }
        return (a.right - a.left) < (b.right - b.left);
    };
    auto it = lower_bound(surfaces.begin(), surfaces.end(), nw, compare);
    surfaces.insert(it, nw);
}

void update_surfaces(VS& surfaces, surface& srf, int left, int right, const P or_shape, int amount){
    /*This function updates the possible surface where new rectangles can be placed. This means that:
    a)If placing a rectangle makes two surfaces be on the same level, merge them into a single surface
    b)If a surface isn't completely filled, include the residue as a new surface
    INPUT: surfaces (vector of all available surfaces), srf (surface where the rectangle is being placed),
    left (surfaces index where left neighbour to srf is found), right (same as left but to the right side),
    or_shape (piece being placed), amount (how many pieces are being placed at once)
    */

   //Editar si considerem esquerra o dreta o generar una fucnio per a cada.
    if(left != -1 and surfaces[left].level==srf.level+or_shape.second){
        surfaces[left].right=srf.left+or_shape.first*amount;
        //If the new surface can merge with the right surface
        if(right != -1 and surfaces[left].level==surfaces[right].level and surfaces[left].right==surfaces[right].left){
            surfaces[left].right = surfaces[right].right;
            surfaces.erase(surfaces.begin()+right);
        }
    }
    else{
        surface new_srf {srf.level + or_shape.second,srf.left,srf.left+or_shape.first*amount};
        if(right != -1 and new_srf.level==surfaces[right].level and new_srf.right==surfaces[right].left){
            surfaces[right].left = new_srf.left;
        }
        else{
            L = max(L,new_srf.level);
            add_surface(surfaces,new_srf);
        }
    }
    if(or_shape.first*amount!=srf.right-srf.left){
        srf.left+=or_shape.first*amount;
        add_surface(surfaces,srf);
    }
}

void empty_filler(VS& surfaces, surface srf, int left, int right){
    /*Given a surface and its neighbours, raise the surfaces level to match the neighbour with the
    least level and merge them into one.If both neighbours have the same level merge all three 
    surfaces into one.
    INPUT: surfaces (vector of surfaces), srf (surface to merge), 
    left and right (indexes of the left and right neighbours in surfaces vec).
    */
   //If both neighbours exists (not equal to -1) and have the same level, merge into a single surface
    if(right!=-1 and left!=-1 and surfaces[left].level == surfaces[right].level){
        surface mod_srf = surfaces[left];
        mod_srf.right = surfaces[right].right;
        surfaces.erase(surfaces.begin()+min(left,right));
        surfaces.erase(surfaces.begin()+max(left,right)-1);
        add_surface(surfaces,mod_srf);
    }
    //If the right neighbour exists (not equal to -1) and has the least height, merge into the right
    else if(right != -1 and (left == -1 or surfaces[left].level>surfaces[right].level)){
        surface mod_srf = surfaces[right]; 
        surfaces.erase(surfaces.begin()+right);
        mod_srf.left = srf.left;
        add_surface(surfaces,mod_srf);
    }
    //Merge into the left 
    else{
        surface mod_srf = surfaces[left]; 
        surfaces.erase(surfaces.begin()+left);
        mod_srf.right = srf.right;
        add_surface(surfaces,mod_srf);
    }
}

void greedy(){
    /*This funciton finds a solution to the problem.
    It propritizes the surfaces onto which pieces can be placed that rest lower (their level is smaller).
    Then following the heuristic: width + 0.5*height, finds the best shape/s (multiple shapes of the same 
    type and same rotation can be placed at once) that fit into the gap. If no place is found the surface 
    is merged onto the closest surface (from the ones next to it in the x axis, the one with the smallest level).
    This process is repeated until there are no pieces left to place.
    */
    surface base {0,0,W};
    VS surfaces;
    surfaces.push_back(base);
    while(N!=0){
        //Delete current surface
        surface srf = surfaces.front();
        surfaces.erase(surfaces.begin());
        P lfnrg = neighbours(surfaces,srf);
        int left = lfnrg.first;
        int right = lfnrg.second;
        //Generate optimal shape to place
        P shape_specs = optimal_shape(srf.right-srf.left);
        //If a shape has been found
        if(shape_specs.second != -1){
            P coord = make_pair(srf.left,srf.level);
            P or_shape = shapes[shape_specs.second].first;
            //Check piece rotation
            if(shape_specs.first <0) {swap(or_shape.first,or_shape.second); shape_specs.first = -shape_specs.first;}

            //Vols que considerem posarles dreta o esquerra en funció dels veins
            //El document ho menciona, aixi que vols fer
            //S'hauria d'aplicar en el següent bloc i tindre en compte en el de construcció de noves surfaces

            //Add the placed pieces to the solution and update pool of possible pieces
            for(int i=0;i<shape_specs.first;++i){
                addtosol(coord,or_shape);
                --N;
                --shapes[shape_specs.second].second;
                coord.first += or_shape.first;
            }
            update_surfaces(surfaces,srf,left,right,or_shape,shape_specs.first); 
        }
        //If no shape has been found
        else{
            empty_filler(surfaces,srf,left,right);
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