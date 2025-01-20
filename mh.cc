#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <climits>
#include <ctime>
#include<algorithm>
#include <random>

using namespace std;

struct surface{
    int level;
    int left;
    int right;
};

typedef pair<int, int> P;
typedef vector<surface> VS;
typedef pair<P, int>    PI;
typedef vector<P>       VP;
typedef vector<PI>     VPI;
typedef pair<P, P>      PP;
typedef vector<PP>     VPP;

ifstream in;
clock_t start;
int  W, N;
int L = INT_MAX;
int C = 0;
double extra_time = 0;
vector<int> ns;
VPI shapes;
VPP  sol;
char* fileName;

//Random generator
random_device rd;
mt19937 gen(rd());

//Parameters to tune the GRASP algorithm
//Delta defines which (1-DELTA) percent of the best matches should the random answer come from
double delta;

//Parameter to tune during how much time the GRASP algorithm is executed.

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
    ns.push_back(ni);
    C += max(pi,qi)*ni;
    shapes.push_back(make_pair(make_pair(pi,qi),ni));
  }
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
    out.precision(1);
    out << duration << endl << L << endl;
    for( PP coords : sol){
        out << coords.first.first << ' ' << coords.first.second << ' ';
        out << coords.second.first << ' ' << coords.second.second << endl;
    }
    out.close();
}

void create_sup(surface& suf, int level, int left, int right){
    suf.level = level;
    suf.left = left;
    suf.right = right;
}

void addtoparsol(const P& coord, const P& shape, VPP& par_sol){
    /* Adds new element to the partial solution, where the elements are ordered by 
    the height they reach.
    */
    PP entry = make_pair(coord,make_pair(coord.first+shape.first-1, coord.second+shape.second-1));
    auto compare = [](const PP& a, const PP& b){
        return a.second.second < b.second.second;
    };
    auto it = lower_bound(par_sol.begin(),par_sol.end(), entry, compare);
    par_sol.insert(it,entry);
}

void sort_shapes(VP& pool){
    /*Sorts the vector of shapes using the heuristic width + 0.5*height.
    Remainder that the pairs com as number of pieces (sign indicating orientation) and index of reference of the piece
    INPUT: pool (vector of the pieces identificator (number and orinetation, index of reference))
    */
   sort(pool.begin(), pool.end(), [&](const auto &a, const auto &b) {
    auto comput = [&](const auto &elem) {
        if (elem.first < 0) {
            return shapes[elem.second].first.second * (-elem.first) + 0.5 * shapes[elem.second].first.first;
        } else {
            return shapes[elem.second].first.first * elem.first + 0.5 * shapes[elem.second].first.second;
        }
    };
    return comput(a) > comput(b);
    });
}

P optimal_shape(VPI& pieces, int width, bool isimproving=false){
    /*Finds the optimal piece, using the max(widht + 0.5*height) criterium, to be placed unto the surface
    INPUT: VPI of the available pieces, width of the surface, bool (indicates whether to account for rotation or not,
    this is because in the improving phase we disregard rotation and only seatch a single rotation). 
    OUTPUT: pair<int,int> that signify:
    1) The first int showcases the amount of times a piece has been placed and its sign whether it has been rotated or not (negative implies rotation)
    2) Index in shapes that identifies the piece being placed
    */
    P chosen = make_pair(0,-1);
    VP pool;
    int maxim = 1;
    for(int j=0;j<int(pieces.size());++j){
        PI type = pieces[j];
        P shape = type.first;
        P no_rot = make_pair(-1,-1);
        P rot = make_pair(-1,-1);
        for(int i=1;i<=type.second;++i){
            //Non rotated shape
            if(shape.first*i<=width){
                no_rot = make_pair(i,j);
            }
            if (not isimproving) {
            //Rotated shape
                if(shape.second*i<=width){
                    rot = make_pair(-i,j);
                }
            }
        }
        if(no_rot.second != -1) pool.push_back(no_rot);
        if(rot.second != -1) pool.push_back(rot);
    }
    //If no shape has been found
    if(int(pool.size())==0) return chosen;

    sort_shapes(pool);

    //During the improvement phase only the best candidate is chosen
    double oldelta;
    if (isimproving) {
        oldelta = delta;
        delta = 1;
    }

    //Get the pool of best (1-delta)% possible candidates
    maxim = max(maxim, int(pool.size()*(1-delta)));
    VP choice_pool;
    for(int i=0;i<maxim;++i){
        choice_pool.push_back(pool[i]);
    }

    uniform_int_distribution<> dist(0,maxim-1);
    int random_index = dist(gen);
    if (isimproving) delta = oldelta;
    return choice_pool[random_index];      
}

P neighbours(VS& surfaces, surface srf){
    /*Pair of left and right indexes of the position of the neighbouring surfaces in surfaces vector
    INPUT: surfaces (vector of surfaces), srf (surface whose neighbour it finds)
    OUTPUT: pair<int,int> of the left and right indexes of the neigbouring surfaces in the surfaces vector
    */
    int left = -1;
    int right = -1;
    for (int i = 0; i < int(surfaces.size()); ++i) {
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
    /*Adds a new surface into the surfaces vector such that it stays sorted 
    (sorted according to ascending level and then by ascending width)
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

void update_surfaces(VS& surfaces, surface& srf, int left, int right, const P or_shape, int amount, int& l){
    /*This function updates the possible surface where new rectangles can be placed. This means that:
    a)If placing a rectangle makes two surfaces be on the same level, merge them into a single surface
    b)If a surface isn't completely filled, include the residue as a new surface
    INPUT: surfaces (vector of all available surfaces), srf (surface where the rectangle is being placed),
    left (surfaces index where left neighbour to srf is found), right (same as left but to the right side),
    or_shape (piece being placed), amount (how many pieces are being placed at once)
    */

    //Checks if the new surface can be merged with left nieghbouring surface
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
        //Checks if the new surface can be merged with the right surface
        if(right != -1 and new_srf.level==surfaces[right].level and new_srf.right==surfaces[right].left){
            surfaces[right].left = new_srf.left;
        }
        else{
            l = max(l,new_srf.level);
            add_surface(surfaces,new_srf);
        }
    }
    //If the old surface has not been filled, add thee remaining part
    if(or_shape.first*amount!=srf.right-srf.left){
        srf.left+=or_shape.first*amount;
        add_surface(surfaces,srf);
    }
}

void empty_filler(VS& surfaces, surface srf, int left, int right){
    /*Given a surface unto which no piece can be placed, fill the empty spaces such that it can merge with
    the nearest neighbouring surface (that with the least level)
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
    //Merge into the left neighbour
    else{
        surface mod_srf = surfaces[left]; 
        surfaces.erase(surfaces.begin()+left);
        mod_srf.right = srf.right;
        add_surface(surfaces,mod_srf);
    }
}

void greedy(int n, VPP& par_sol, VS& surfaces, int& l){
    /* Constructive phase of the GRASP algorithm. 
    It propritizes the surfaces onto which pieces can be placed that rest lower (their level is smaller).
    Then following the heuristic: width + 0.5*height, chooses randomly among the (1-delta)% best shape/s 
    (multiple shapes of the same type and same rotation can be placed at once) that fit into the gap. If 
    no place is found the surface is merged onto the closest surface (from the ones next to it in the x 
    axis, the one with the smallest level). This process is repeated until there are no pieces left to place.
    INPUT: n (number of shapes left to place), par_sol (vector of the curretnly placed shapes),
    surfaces (vector of current surfaces), l (height of the current solution)
    */
    surface base {0,0,W};
    surfaces.push_back(base);
    while(n>0){
        //Delete current surface
        surface srf = surfaces.front();
        surfaces.erase(surfaces.begin());
        P lfnrg = neighbours(surfaces,srf);
        int left = lfnrg.first;
        int right = lfnrg.second;
        //Generate random piece among the most optimal
        //If among the k% last, choose always the best piece
        P shape_specs = optimal_shape(shapes,srf.right-srf.left,false);
        //If a shape has been found
        if(shape_specs.second != -1){
            P coord = make_pair(srf.left,srf.level);
            P or_shape = shapes[shape_specs.second].first;
            //Check piece rotation
            if(shape_specs.first <0) {swap(or_shape.first,or_shape.second); shape_specs.first = -shape_specs.first;}
                
            //Add the placed pieces to the solution and update pool of possible pieces
            for(int i=0;i<shape_specs.first;++i){
                addtoparsol(coord,or_shape,par_sol);
                --n;
                --shapes[shape_specs.second].second;
                coord.first += or_shape.first;
            }
            update_surfaces(surfaces,srf,left,right,or_shape,shape_specs.first,l); 
        }
        //If no shape has been found
        else{
            empty_filler(surfaces,srf,left,right);
        }
    }
}

int height (const PP& piece) {
    /*Given a shape following the solution standard, return the shape's height
    INPUT: piece (pair of the left upper and right bottom coordinates)
    OUTPUT: integer (height of the piece)
    */
    return piece.second.second - piece.first.second+1;
}

int width (const PP& piece) {
    /*Given a shape following the solution standard, return the shape's width
    INPUT: piece (pair of the left upper and right bottom coordinates)
    OUTPUT: integer (width of the piece)
    */
    return piece.second.first -piece.first.first+1;
}

void generate(VS& surfaces, vector<int>& v, int left){
    /* Given a vector v of non-negative integers, and a left value, transforms the consecutive
     values in v into their own separate surfaces, where its level is the value in v, and its 
     left and right atributes correspond to its relative position within the vector v offset by 
     the value left. Once the surface is created is added to the vector of surfaces accounting 
     for mergers.
     INPUT: surfaces (vector of surfaces), v (vector of level values),
     left (left attribute of the original surface) 
    */
    int n = 0;
    VPI final;
    final.push_back(make_pair(make_pair(0,1),v[n]));
    int currentindex = 0;
    ++n;
    while(n != int(v.size())) {
        if (v[n] == final[currentindex].second) {
            ++final[currentindex].first.second;
        }
        else {
            ++currentindex;
            final.push_back(make_pair(make_pair(n,n+1),v[n]));
        }
        ++n;
    }
    for(PI newone : final){
        surface srf;
        srf.level = newone.second;
        srf.left = newone.first.first+left;
        srf.right = newone.first.second+left;
        P veins = neighbours(surfaces,srf);
        int melasuda = 0;
        update_surfaces(surfaces,srf,veins.first,veins.second,make_pair(srf.right-srf.left,0),1,melasuda);
    }
}

void metagreedy (int n, VPI& shapesimprove, VS& surfaces, int&l,VPP& par_sol) {
    /* Variant of the constructive phase where rotation is disregarded and the 
    available shapes are limited.
    It propritizes the surfaces onto which pieces can be placed that rest lower (their level is smaller).
    Then following the heuristic: width + 0.5*height, chooses the best shape that fit into the gap. If 
    no place is found the surface is merged onto the closest surface (from the ones next to it in the x 
    axis, the one with the smallest level). This process is repeated until there are no pieces left to place.
    INPUT: n (number of shapes left to place), shapesimprove (pool of available shapes to place), 
    surfaces (vector of current surfaces), l (height of the current solution), 
    par_sol (vector of the curretnly placed shapes)
    */
    while(n>0){
        //Delete current surface
        surface srf = surfaces.front();
        surfaces.erase(surfaces.begin());
        P lfnrg = neighbours(surfaces,srf);
        int left = lfnrg.first;
        int right = lfnrg.second;

        /*let's convert the pieces into a VPI (shape and amount = 1)*/
        
        P shape_specs = optimal_shape(shapesimprove,srf.right-srf.left,true);
        //If a shape has been found
        if(shape_specs.second != -1){
            P coord = make_pair(srf.left,srf.level);
            P or_shape = shapesimprove[shape_specs.second].first;
            addtoparsol(coord,or_shape,par_sol);
            --n;
            shapesimprove.erase(shapesimprove.begin() + shape_specs.second);
            if(left == right and left == -1) {}
            update_surfaces(surfaces,srf,left,right,or_shape,shape_specs.first,l); 
        }
        //If no shape has been found
        else{
            if(left == right and left == -1){
                return;
            }
            empty_filler(surfaces,srf,left,right);
        }
    }

}

bool improve(VPP& par_sol, VS& surfaces, int& old_l){
    /*Improvement phase of the GRASP algorithm
    It deletes the shapes that are part of the top-most surface. Once deleted they are rotated
    and placed again. If this new answer is better than the previous answer it replaces the
    previous one.

    INPUT: par_sol (vector of placed pieces ordered by heighest reaching), 
    surfaces (vector of avialable surfaces ordered by level), 
    old_l (heighest level on the board in the previous answer)
    OUTPUT: bool (showcases whether an improved solution has been found)
    */
    surface s = surfaces.back();
    surfaces.pop_back();
    VPI shapesinsurface;
    int wide = s.right - s.left;

    //Vector of levels to repopulate the surfaces
    vector<int> heights (wide,0);

    int i = par_sol.size()-1;

    //Find the shapes that are part of top-mosst surface
    //Gather data necessary to repopulate the surfaces as shapes are being removed
    while (wide!=0 and i >= 0) {
        if(par_sol[i].first.first>=s.left or par_sol[i].second.first + 1 <= s.right){
            if(par_sol[i].second.second+1==s.level){
                int h;
                PP par = par_sol[i];
                if(height(par)>width(par) and height(par)<=W){
                    h = par.first.second;
                    PI rotated;
                    rotated.second = 1;
                    rotated.first = make_pair(height(par),width(par));
                    shapesinsurface.push_back(rotated);
                    par_sol.erase(par_sol.begin()  + i);
                }
                else{
                    h = s.level;
                }
                for(int j = max(0,par.first.first-s.left);j<min(par.first.first + width(par) - s.left,s.right-s.left);++j){
                    heights[j] = h;
                    --wide;
                }
            }
            else for(int j = max(0,par_sol[i].first.first-s.left);j<min(s.right-s.left,par_sol[i].first.first + width(par_sol[i]) - s.left);++j){
                if(heights[j]==0) {heights[j]= par_sol[i].second.second +1; --wide;}
            }
        }
        --i;
    }

    //Repopulate the missing surfaces
    generate(surfaces,heights,s.left);

    //Generate the "improved" answer and check if it is better
    int l;
    if(int(par_sol.size())!=0) l = par_sol.back().second.second + 1;
    else l=0;
    metagreedy(int(shapesinsurface.size()),shapesinsurface,surfaces,l,par_sol);
    l = par_sol.back().second.second + 1;

    if(l<old_l){
        old_l = l;
        return true;
    }
    return false;
}
double delta_calculation (int N){
    /*Usually, scientific evidence says 0.75 is optimal, but since we're working with a finite amount of time
    and reasonably small inputs, we can adapt it in order to generate better solutions. As it is expected, as we increase
    the amount of pieces we increase the delta, since otherwise it would be difficult to approximate the convergence value */
    /*INPUT: number of pieces in the .inp file*/
    /*OUTPUT: Delta parameter to use*/

    if (N < 20 ) return 0.2;
    else if (N < 40) return 0.5;
    else if (N < 90) return 0.75;
    else return 0.9;
}

void grasp(){
    /*GRASP algorithm implementation.
    It iterates trying to find the best possible answer.
    At each iteration generates a random semi-optimal solution and tries to improve it.
    It only retains the best answer found to date.
    */
    
    /*Let us define the delta we're going to use for the iterations*/
    delta = delta_calculation(N);

    while(true){

        VPP par_sol;
        VS surfaces;
        int l = 0;

        //Constructive part
        greedy(N,par_sol,surfaces,l);

        //If better answer has been found, write it
        if(l<L){
            sol = par_sol;
            L = l;
            write_instance(fileName);
        }

        //Improvement part
        //It will improve until it is no longer possible
        while(improve(par_sol,surfaces,l)){
            
        };

        //If better answer, write it down
        if(l<L){
                sol = par_sol;
                L = l;
                write_instance(fileName);
            }


        //Resets values of shapes for next iteration
        for(int j=0;j<int(shapes.size());++j){
            shapes[j].second = ns[j];
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
  fileName = argv[2];

  grasp();
}