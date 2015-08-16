/*
 * ellipsoid_fit.h
 *
 *  Created on: Jul 24, 2015
 *      Author: antonermakov
 */

#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/function.h>
#include <deal.II/base/logstream.h>
#include <deal.II/lac/vector.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/compressed_sparsity_pattern.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/precondition.h>
#include <deal.II/grid/tria.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_tools.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_values.h>
#include <deal.II/numerics/vector_tools.h>
#include <deal.II/numerics/matrix_tools.h>
#include <deal.II/numerics/data_out.h>
#include <deal.II/grid/grid_in.h>
#include <deal.II/grid/grid_out.h>
#include <deal.II/grid/tria_boundary_lib.h>
#include <deal.II/base/point.h>
#include <deal.II/grid/grid_generator.h>

#include <fstream>
#include <sstream>


#include "local_math.h"

using namespace dealii;

template <int dim>
class ellipsoid_fit
{
public:
       inline ellipsoid_fit (Triangulation<dim,dim>* pi)
       {
    	   p_triangulation = pi;
       };
  void compute_fit(std::vector<double> &ell, unsigned char bndry);

private:
  Triangulation<dim,dim>*   p_triangulation;

};


// This function computes ellipsoid fit to a set of vertices that lie on the
// boundary_that_we_need
template <int dim>
void ellipsoid_fit<dim>::compute_fit(std::vector<double> &ell, unsigned char boundary_that_we_need)
{
	typename Triangulation<dim>::active_cell_iterator cell = p_triangulation->begin_active();
    typename Triangulation<dim>::active_cell_iterator endc = p_triangulation->end();

    FullMatrix<double> A(p_triangulation->n_vertices(),dim);
	Vector<double>     x(dim);
	Vector<double>     b(p_triangulation->n_vertices());

	std::vector<bool> vertex_touched (p_triangulation->n_vertices(),
			                                     false);

    unsigned int j = 0;
    unsigned char boundary_ids;
    std::vector<unsigned int> ind_bnry_row;
    std::vector<unsigned int> ind_bnry_col;

	// assemble the sensitivity matrix and r.h.s.
	for (; cell!=endc; ++cell)
	    for (unsigned int f=0; f < GeometryInfo<dim>::faces_per_cell; ++f)
		{
	        boundary_ids = cell->face(f)->boundary_indicator();
			if(boundary_ids == boundary_that_we_need)
			{
			    for (unsigned int v=1; v<GeometryInfo<dim>::vertices_per_face; ++v)
				    if (vertex_touched[cell->face(f)->vertex_index(v)] == false)
					{
					    vertex_touched[cell->face(f)->vertex_index(v)] = true;
						for (unsigned int i=0; i<dim; ++i)
						{
						    // sensitivity matrix entry
						    A(j,i) = pow(cell->face(f)->vertex(v)[i],2);
						    // r.h.s. entry
						    b[j] = 1.0;
						    // if mesh if not full: set the indicator
						}
					    ind_bnry_row.push_back(j);
						j++;
					}
			}
		}

	// maxtrix A'*A and vector A'*b;  A'*A*x = A'*b -- normal system of equations
    FullMatrix<double> AtA(dim,dim);
	Vector<double>     Atb(dim);

	FullMatrix<double> A_out(ind_bnry_row.size(),dim);
	Vector<double>     b_out(ind_bnry_row.size());

	for(unsigned int i=0;i<dim;i++)
		ind_bnry_col.push_back(i);

    for(unsigned int i=0;i<ind_bnry_row.size();i++)
	    b_out(i) = 1;

    A_out.extract_submatrix_from(A, ind_bnry_row, ind_bnry_col);
    A_out.Tmmult(AtA,A_out,true);
    A_out.Tvmult(Atb,b_out,true);

	// solve normal system of equations
    SolverControl           solver_control (1000, 1e-12);
    SolverCG<>              solver (solver_control);
    solver.solve (AtA, x, Atb, PreconditionIdentity());

	 // find ellipsoidal axes
	 for(unsigned int i=0; i<dim; i++)
         ell.push_back(sqrt(1.0/x[i]));

}

