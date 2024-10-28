#include"toolsmatrix.hpp"
#include "Timer.hpp"
int main() {
  //int N;  
  //std::cout<<"Set the size of Matrix[size][size]"<<std::endl;
  //std::cin>>N;
  int N{1000};
  // to have different matrices
  std::srand(time(0));
  CMatrix<double> A(N), B(N);
  A.fill();
  B.fill();  
  //std::cout << "A is" << std::endl;
  //std::cout << A << std::endl;
  //std::cout << "B is" << std::endl;
  //std::cout << B << std::endl;
 
  {CSimple_timer t("Matrix mult.");
  auto C = A*B;
  }
  CSimple_timer::print_timing_results();
  //std::cout<<A*B<<std::endl;
  /*
  std::cout << "A is" << std::endl;
  std::cout << A << std::endl;
  std::cout << "B is" << std::endl;
  std::cout << B << std::endl;
  std::cout << "A*B is" << std::endl;
  std::cout << A * B << std::endl;
  std::cout << "sum is" << std::endl;
  std::cout << A + B << std::endl;
  auto C = A*B;
  std::cout<<"C=A*B is"<<std::endl;
  std::cout<<C<<std::endl;
  auto D = A + B;
  std::cout << "D=A+B is" << std::endl;
  std::cout << D << std::endl;
  auto E(A);
  std::cout<<"copy of A is "<<std::endl;
  std::cout<<E<<std::endl;
  E=B;
  std::cout<<"copy of B is "<<std::endl;
  std::cout<<E<<std::endl;
  auto E2(std::move(C));
  std::cout<<"copy of C is"<<std::endl;
  std::cout<< E2 << std::endl;
  std::cout<<"can you print C after this? What happens?"<<std::endl;
  //std::cout<<C<<std::endl;// This is dangerous, after to move we cann´t use C anymore  
                          // give me "Segmentation fault (core dumped)"
  */
  return 0;
}
