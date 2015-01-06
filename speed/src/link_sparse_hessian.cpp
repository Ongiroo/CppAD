/* $Id: link_sparse_hessian.cpp 3223 2014-03-19 15:13:26Z bradbell $ */
/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-15 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the 
                    Eclipse Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */

/*
$begin link_sparse_hessian$$
$spell
	const
	bool
	CppAD
	cppad
	colpack
$$

$index link_sparse_hessian$$
$index sparse, speed test$$
$index speed, test sparse$$
$index test, sparse speed$$

$section Speed Testing Sparse Hessian$$

$head Prototype$$
$codei%extern bool link_sparse_hessian(
	size_t                        %size%      ,
	size_t                        %repeat%    ,
	CppAD::vector<double>&        %x%         ,
	const CppAD::vector<size_t>&  %row%       ,
	const CppAD::vector<size_t>&  %col%       , 
	CppAD::vector<double>&        %hessian%   ,
	size_t                        %n_sweep%
);
%$$

$head Method$$
Given a row index vector $latex row$$
and a second column vector $latex col$$,
the corresponding function 
$latex f : \B{R}^n \rightarrow \B{R} $$ 
is defined by $cref sparse_hes_fun$$.
The non-zero entries in the Hessian of this function have 
one of the following forms:
$latex \[
	\DD{f}{x[row[k]]}{x[row[k]]}
	\; , \;
	\DD{f}{x[row[k]]}{x[col[k]]}
	\; , \;
	\DD{f}{x[col[k]]}{x[row[k]]}
	\; , \;
	\DD{f}{x[col[k]]}{x[col[k]]}
\] $$
for some $latex k $$ between zero and $latex K-1 $$.
All the other terms of the Hessian are zero.

$head size$$
The argument $icode size$$, referred to as $latex n$$ below,
is the dimension of the domain space for $latex f(x)$$.

$head repeat$$
The argument $icode repeat$$ is the number of times
to repeat the test
(with a different value for $icode x$$ corresponding to
each repetition).

$head x$$
The argument $icode x$$ has prototype
$codei%
        CppAD::vector<double>& %x%
%$$
and its size is $latex n$$; i.e., $icode%x%.size() == %size%$$.
The input value of the elements of $icode x$$ does not matter.
On output, it has been set to the
argument value for which the function,
or its derivative, is being evaluated.
The value of this vector need not change with each repetition.

$head row$$
The argument $icode row$$ has prototype
$codei%
	const CppAD::vector<size_t> %row%
%$$
Its size defines the value $latex K$$.
It contains the row indices for the corresponding function $latex f(x)$$.
All the elements of $icode row$$ are between zero and $latex n-1$$.

$head col$$
The argument $icode col$$ has prototype
$codei%
	const CppAD::vector<size_t> %col%
%$$
Its size must be the same as $icode row$$; i.e., $latex K$$.
It contains the column indices for the corresponding function 
$latex f(x)$$.
All the elements of $icode col$$ are between zero and $latex n-1$$.
$pre

$$
There are no duplicate row and column entires; i.e., if $icode%j% != %k%$$,
$codei%
	%row%[%j%] != %row%[%k%] || %col%[%j%] != %col%[%k%]
%$$
Only the lower triangle of the Hessian is included in the indices; i.e.,
$codei%row%[%k%] >= %col%[%k%]%$$.
Furthermore, for all the non-zero entries in the lower triangle
are included; i.e., if $latex i \geq j$$ and
$latex \DD{f}{x[i]}{x[j]} \neq 0$$,
there is an index $icode k$$ such that
$icode%i% = %row%[%k%]%$$ and
$icode%j% = %col%[%k%]%$$.

$head hessian$$
The argument $icode hessian$$ has prototype
$codei%
	CppAD::vector<double>&  hessian
%$$
and its size is $latex n \times n$$.
The input value of its elements does not matter. 
The output value of its elements is the Hessian of the function $latex f(x)$$.
To be more specific, for
$latex k = 0 , \ldots , K-1$$,
$latex \[
	\DD{f}{x[row[k]]}{x[col[k]]} (x) = hessian [k] 
\] $$

$head n_sweep$$
The input value of $icode n_sweep$$ does not matter. On output,
it is the value $cref/n_sweep/sparse_hessian/n_sweep/$$ corresponding
to the evaluation of $icode hessian$$.
This is also the number of colors corresponding to the 
$cref/coloring method/sparse_hessian/work/color_method/$$,
which can be set to $cref/colpack/speed_main/Sparsity Options/colpack/$$,
and is otherwise $code cppad$$.


$subhead double$$
In the case where $icode package$$ is $code double$$,
only the first element of $icode hessian$$ is used and it is actually 
the value of $latex f(x)$$ (derivatives are not computed).

$end 
-----------------------------------------------------------------------------
*/
# include <cppad/vector.hpp>
# include <cppad/near_equal.hpp>
# include <cppad/speed/sparse_hes_fun.hpp>
# include <cppad/speed/uniform_01.hpp>
# include <cppad/index_sort.hpp>

/*!
\{
\file link_sparse_hessian.cpp
Defines and implement sparse Hessian speed link to package specific code.
*/
namespace {
	using CppAD::vector;

	/*!
 	Class used by choose_row_col to determin order of row and column indices
	*/
	class Key {
	public:
		/// row index
		size_t row_;
		/// column index
		size_t col_;
		/// default constructor
		Key(void)
		{ }
		/*!
 		Construct from a value for row and col

		\param row
		row value for this key

		\param col
		column value for this key
 		*/
		Key(size_t row, size_t col)
		: row_(row), col_(col)
		{ }
		/*!
 		Compare this key with another key using < operator

		\param other
		the other key.
		*/
		bool operator<(const Key& other) const
		{	if( row_ == other.row_ )
				return col_ < other.col_;
			return row_ < other.row_;
		}
	};

	/*!
	Function that randomly choose the row and column indices

	\param n [in]
	is the dimension of the argument space for the function f(x).

	\param row [out]
	the input size and elements of \c row do not matter.
	Upon return it is the chosen row indices.

	\param col [out]
	the input size and elements of \c col do not matter.
	Upon return it is the chosen column indices.
	*/
	void choose_row_col(
		size_t          n   ,
		vector<size_t>& row , 
		vector<size_t>& col )
	{	size_t r, c, k, K = 5 * n;

		// get the random indices
		vector<double>  random(2 * K);
		CppAD::uniform_01(2 * K, random);

		// sort the temporary row and colunn choices
		vector<Key>   keys(K + n);
		vector<size_t> ind(K + n);
		for(k = 0; k < K; k++)
		{	r = size_t( n * random[k] );
			r = std::min(n-1, r);
			//
			c = size_t( n * random[k + K] );
			c = std::min(n-1, c);
			//
			// force to lower triangle
			if( c > r )
				std::swap(r, c);
			//
			keys[k] = Key(r, c);
		}
		// include the diagonal
		for(k = 0; k < n; k++)
			keys[k + K] = Key(n / 2, k);  
		CppAD::index_sort(keys, ind);

		// remove duplicates while setting the return value for row and col
		row.resize(0);
		col.resize(0);
		size_t r_previous = keys[ ind[0] ].row_;
		size_t c_previous = keys[ ind[0] ].col_;
		row.push_back(r_previous);
		col.push_back(c_previous);
		for(k = 1; k < K; k++)
		{	r = keys[ ind[k] ].row_;
			c = keys[ ind[k] ].row_;
			if( r != r_previous || c != c_previous)
			{	row.push_back(r);
				col.push_back(c);
			}
			r_previous = r;
			c_previous = c;
		}
	}
}

/*!
Package specific implementation of a sparse Hessian claculation.

\param size [in]
is the size of the domain space; i.e. specifies \c n.

\param repeat [in]
number of times tha the test is repeated.

\param x [out]
is a vector of size \c n containing
the argument at which the Hessian was evaluated during the last repetition.

\param row [in]
is the row indices correpsonding to non-zero Hessian entries.

\param col [in]
is the column indices corresponding to non-zero Hessian entries.

\param hessian [out]
is a vector with size <code>n * n</code> 
containing the value of the Hessian of f(x) 
corresponding to the last repetition.

\param n_sweep [out]
The input value of this parameter does not matter.
Upon return, it is the number of sweeps (colors) corresponding
to the sparse hessian claculation.

\return
is true, if the sparse Hessian speed test is implemented for this package,
and false otherwise.
*/
extern bool link_sparse_hessian(
	size_t                           size      ,
	size_t                           repeat    ,
	const CppAD::vector<size_t>&     row       ,
	const CppAD::vector<size_t>&     col       , 
	CppAD::vector<double>&           x         ,
	CppAD::vector<double>&           hessian   ,
	size_t&                          n_sweep
);

/*!
Is sparse Hessian test avaialable.

\return
true, if spare Hessian available for this package, and false otherwise.
*/
bool available_sparse_hessian(void)
{	
	size_t n      = 2;
	size_t repeat = 1;
	vector<double> x(n);
	vector<size_t> row, col; 
	choose_row_col(n, row, col);
	size_t K = row.size();
	vector<double> hessian(K);

	size_t n_sweep;
	return link_sparse_hessian(n, repeat, row, col, x, hessian, n_sweep);
	exit(0);
}
/*!
Does final sparse Hessian value pass correctness test.

\param is_package_double
if true, we are checking function values instead of derivatives.

\return
true, if correctness test passes, and false otherwise.
*/
bool correct_sparse_hessian(bool is_package_double)
{	
	size_t n      = 10;
	size_t repeat = 1;
	vector<double> x(n);
	vector<size_t> row, col;
	choose_row_col(n, row, col);
	size_t K = row.size();
	vector<double> hessian(K);

	size_t n_sweep;
	link_sparse_hessian(n, repeat, row, col, x, hessian, n_sweep);

	size_t order, size;
	if( is_package_double)
	{	order = 0;  // check function value
		size  = 1;
	}
	else
	{	order = 2;     // check hessian value
		size  = K;
	}
	CppAD::vector<double> check(size);
	CppAD::sparse_hes_fun<double>(n, x, row, col, order, check);
	bool ok = true;
	size_t k;
	for( k = 0; k < size; k++)
		ok &= CppAD::NearEqual(check[k], hessian[k], 1e-10, 1e-10);

	return ok;
}

/*!
Sparse Hessian speed test.

\param size
is the dimension of the argument space for this speed test.

\param repeat
is the number of times to repeate the speed test.
*/
void speed_sparse_hessian(size_t size, size_t repeat)
{	size_t n = size;	
	vector<double> x(n);
	vector<size_t> row, col;
	choose_row_col(n, row, col);
	size_t K = row.size();
	vector<double> hessian(K);

	// note that cppad/sparse_hessian.cpp assumes that x.size() == size
	size_t n_sweep;
	link_sparse_hessian(n, repeat, row, col, x, hessian, n_sweep);
	return;
}

/*!
Sparse Hessian speed test information.

\param size [in]
is the \c size parameter in the corresponding call to speed_sparse_jacobian.

\param n_sweep [out]
The input value of this parameter does not matter.
Upon return, it is the value \c n_sweep retruned by the corresponding
call to \c link_sparse_jacobian.
*/
void info_sparse_hessian(size_t size, size_t& n_sweep)
{	size_t n      = size;	
	size_t repeat = 1;
	vector<size_t> row, col;
	choose_row_col(n, row, col);

	// note that cppad/sparse_jacobian.cpp assumes that x.size()
	// is the size corresponding to this test
	vector<double> x(n);
	size_t K = row.size();
	vector<double> hessian(K);
	link_sparse_hessian(n, repeat, row, col, x, hessian, n_sweep);
	return;
}

