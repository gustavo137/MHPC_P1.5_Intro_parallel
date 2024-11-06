#include "tools.hpp"
#include "Timer.hpp"
int main(){
 std::vector<double> conini={0,100};
 size_t dim{9};//100// for leonardo set 10000
 size_t ite{10};
 size_t printInterval{200};
 //std::cout<<"Set the size of the matrix[dim][dim]"<<std::endl;
 //std::cin>>dim;
 //Create the mesh class element
 CMesh<double> Matrix(dim, conini);
 //std::cout<<"M is"<<std::endl;
 //Matrix.printf();
 //std::cout<<"Matrix + conditions"<<std::endl; 
 //Matrix.printnf();
 {CSimple_timer t("Jacobi using openmp");
 CSolver<double> solver(Matrix);
 solver.jacobi(Matrix, ite, printInterval);
 }
 //std::cout<<"finished"<<std::endl;
 //call the times
 CSimple_timer::print_timing_results();
 return 0;
}
