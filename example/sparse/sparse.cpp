/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-17 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the
                    Eclipse Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */

// CPPAD_HAS_* defines
# include <cppad/configure.hpp>

// system include files used for I/O
# include <iostream>

// C style asserts
# include <cassert>

// for thread_alloc
# include <cppad/utility/thread_alloc.hpp>

// test runner
# include <cppad/utility/test_boolofvoid.hpp>

// external compiled tests
extern bool subgraph_jac_rev(void);
extern bool subgraph_sparsity(void);
extern bool sub_sparse_hes(void);
extern bool sparsity_sub(void);
extern bool sparse_sub_hes(void);
extern bool sparse_hes(void);
extern bool sparse_jac_for(void);
extern bool sparse_jac_rev(void);
extern bool sparse_jacobian(void);
extern bool sparse_hessian(void);
extern bool rev_hes_sparsity(void);
extern bool rev_jac_sparsity(void);
extern bool RevSparseJac(void);
extern bool rev_sparse_hes(void);
extern bool subgraph_reverse(void);
extern bool for_hes_sparsity(void);
extern bool for_jac_sparsity(void);
extern bool ForSparseJac(void);
extern bool for_sparse_hes(void);
extern bool dependency(void);
extern bool conj_grad(void);
extern bool colpack_jac(void);
extern bool colpack_jacobian(void);
extern bool colpack_hes(void);
extern bool colpack_hessian(void);
extern bool rc_sparsity(void);

// main program that runs all the tests
int main(void)
{	std::string group = "example/sparse";
	size_t      width = 20;
	CppAD::test_boolofvoid Run(group, width);

	// This line is used by test_one.sh

	// external compiled tests
	Run( subgraph_jac_rev,          "subgraph_jac_rev" );
	Run( subgraph_sparsity,         "subgraph_sparsity" );
	Run( sub_sparse_hes,            "sub_sparse_hes" );
	Run( sparsity_sub,              "sparsity_sub" );
	Run( sparse_sub_hes,            "sparse_sub_hes" );
	Run( sparse_hes,                "sparse_hes" );
	Run( sparse_jac_for,            "sparse_jac_for" );
	Run( sparse_jac_rev,            "sparse_jac_rev" );
	Run( sparse_jacobian,           "sparse_jacobian" );
	Run( sparse_hessian,            "sparse_hessian" );
	Run( rev_hes_sparsity,          "rev_hes_sparsity" );
	Run( rev_jac_sparsity,          "rev_jac_sparsity" );
	Run( RevSparseJac,              "RevSparseJac" );
	Run( rev_sparse_hes,            "rev_sparse_hes" );
	Run( subgraph_reverse,          "reverse_subgraph");
	Run( for_hes_sparsity,          "for_hes_sparsity" );
	Run( for_jac_sparsity,          "for_jac_sparsity" );
	Run( ForSparseJac,              "ForSparseJac" );
	Run( for_sparse_hes,            "for_sparse_hes" );
	Run( dependency,                "dependency" );
	Run( conj_grad,                 "conj_grad" );
	Run( rc_sparsity,               "rc_sparsity" );
# if CPPAD_HAS_COLPACK
	Run( colpack_jac,               "colpack_jac" );
	Run( colpack_jacobian,          "colpack_jacobian" );
	Run( colpack_hes,               "colpack_hes" );
	Run( colpack_hessian,           "colpack_hessian" );
# endif
	//
	// check for memory leak
	bool memory_ok = CppAD::thread_alloc::free_all();
	// print summary at end
	bool ok = Run.summary(memory_ok);
	//
	return static_cast<int>( ! ok );
}
