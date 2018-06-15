/* ************************************************************************
 * Copyright 2018 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once
#ifndef TESTING_BICGSTABL_HPP
#define TESTING_BICGSTABL_HPP

#include "utility.hpp"

#include <rocalution.hpp>

using namespace rocalution;

static bool check_residual(float res)
{
    return (res < 1e-2f);
}

static bool check_residual(double res)
{
    return (res < 1e-5);
}

template <typename T>
bool testing_bicgstabl(Arguments argus)
{
    int ndim = argus.size;
    std::string precond = argus.precond;
    unsigned int format = argus.format;
    int l = argus.index;

    // Initialize rocALUTION platform
    init_rocalution();

    // rocALUTION structures
    LocalMatrix<T> A;
    LocalVector<T> x;
    LocalVector<T> b;
    LocalVector<T> e;

    // Matrix format
    A.ConvertTo(format);

    // Generate A
    int* csr_ptr = NULL;
    int* csr_col = NULL;
    T* csr_val   = NULL;

    int nrow = gen_2d_laplacian(ndim, &csr_ptr, &csr_col, &csr_val);
    int nnz = csr_ptr[nrow];

    A.SetDataPtrCSR(&csr_ptr, &csr_col, &csr_val, "A", nnz, nrow, nrow);

    // Move data to accelerator
    A.MoveToAccelerator();
    x.MoveToAccelerator();
    b.MoveToAccelerator();
    e.MoveToAccelerator();

    // Allocate x, b and e
    x.Allocate("x", A.GetN());
    b.Allocate("b", A.GetM());
    e.Allocate("e", A.GetN());

    // b = A * 1
    e.Ones();
    A.Apply(e, &b);

    // Random initial guess
    x.SetRandomUniform(12345ULL, -4.0, 6.0);

    // Solver
    BiCGStabl<LocalMatrix<T>, LocalVector<T>, T> ls;

    // Preconditioner
    Preconditioner<LocalMatrix<T>, LocalVector<T>, T> *p;

    if(precond == "None") p = NULL;
    else if(precond == "Chebyshev")
    {
        // Chebyshev preconditioner

        // Determine min and max eigenvalues
        T lambda_min;
        T lambda_max;

        A.Gershgorin(lambda_min, lambda_max);

        AIChebyshev<LocalMatrix<T>, LocalVector<T>, T> *cheb = new AIChebyshev<LocalMatrix<T>, LocalVector<T>, T>;
        cheb->Set(3, lambda_max / 7.0, lambda_max);

        p = cheb;
    }
    else if(precond == "FSAI") p = new FSAI<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "SPAI") p = new SPAI<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "TNS") p = new TNS<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "Jacobi") p = new Jacobi<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "GS") p = new GS<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "SGS") p = new SGS<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "ILU") p = new ILU<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "ILUT") p = new ILUT<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "IC") p = new IC<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "MCGS") p = new MultiColoredGS<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "MCSGS") p = new MultiColoredSGS<LocalMatrix<T>, LocalVector<T>, T>;
    else if(precond == "MCILU") p = new MultiColoredILU<LocalMatrix<T>, LocalVector<T>, T>;
    else return false;

    ls.Verbose(0);
    ls.SetOperator(A);

    // Set preconditioner
    if(p != NULL)
    {
        ls.SetPreconditioner(*p);
    }

    ls.Init(1e-8, 0.0, 1e+8, 10000);
    ls.SetOrder(l);
    ls.Build();
    ls.Solve(b, &x);

    // Verify solution
    x.ScaleAdd(-1.0, e);
    T nrm2 = x.Norm();

    bool success = check_residual(nrm2);

    // Clean up
    ls.Clear();
    if(p != NULL)
    {
        delete p;
    }

    // Stop rocALUTION platform
    stop_rocalution();

    return success;
}

#endif // TESTING_BICGSTABL_HPP