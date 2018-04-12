#include <iostream>
#include <paralution.hpp>
#include <complex>

#define ValueType std::complex<double>

using namespace paralution;

int main(int argc, char* argv[]) {

#if defined(SUPPORT_MIC)
  return 0;
#endif

  init_paralution();

  LocalVector<ValueType> x;
  LocalVector<ValueType> rhs;
  LocalVector<ValueType> sol;
  LocalMatrix<ValueType> mat;

  mat.MoveToAccelerator();
  x.MoveToAccelerator();
  rhs.MoveToAccelerator();
  sol.MoveToAccelerator();

  mat.ReadFileMTX(std::string(argv[1]));

  x.Allocate("x", mat.get_nrow());
  rhs.Allocate("rhs", mat.get_nrow());
  sol.Allocate("sol", mat.get_nrow());

  // b = A*1
  sol.Ones();
  x.Zeros();//SetRandom((ValueType) 0, (ValueType) 2, time(NULL));
  mat.Apply(sol, &rhs);

  // Iterative Linear Solver
  IterativeLinearSolver<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType > *ils = NULL;

  std::string sName = argv[2];
  if (sName == "CG")        ils = new CG<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType>;
  if (sName == "CR")        ils = new CR<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType>;
  if (sName == "BiCGStab")  ils = new BiCGStab<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType>;
  if (sName == "GMRES")     ils = new GMRES<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType>;
  if (sName == "FGMRES")    ils = new FGMRES<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType>;

  // Preconditioner
  Preconditioner<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType > *p = NULL;

  ils->SetOperator(mat);
  ils->Init(1e-8, 0.0, 1e+8, 15000);

  std::string pName = argv[3];

  if (pName == "ILU")    p = new ILU<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType>;
  if (pName == "MCILU")  p = new MultiColoredILU<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType>;
  if (pName == "FSAI")   p = new FSAI<LocalMatrix<ValueType>, LocalVector<ValueType>, ValueType>;

  if (p != NULL)
    ils->SetPreconditioner(*p);

  ils->Build();

  std::string mFormat = argv[4];

  if (mFormat == "CSR") mat.ConvertTo(CSR);
  if (mFormat == "MCSR") mat.ConvertTo(MCSR);
  if (mFormat == "COO") mat.ConvertTo(COO);
  if (mFormat == "ELL") mat.ConvertTo(ELL);
  if (mFormat == "DIA") mat.ConvertTo(DIA);
  if (mFormat == "HYB") mat.ConvertTo(HYB);

  mat.info();

  ils->Solve(rhs, &x);

  x.ScaleAdd((ValueType) -1, sol);
  std::cout << "Error Norm = " << x.Norm() << std::endl;

  if (x.Norm().real() > 1e-4) {
    std::cout << "Test failed." << std::endl;
    exit(-1);
  }

  stop_paralution();

  return 0;

}