/***************************************************************************
* ALPS++/lattice library
*
* lattice/lattice.h    the lattice class
*
* $Id$
*
* Copyright (C) 2001-2003 by Matthias Troyer <troyer@comp-phys.org>
*                            Synge Todo <wistaria@comp-phys.org>
*
* Permission is hereby granted, free of charge, to any person or organization 
* obtaining a copy of the software covered by this license (the "Software") 
* to use, reproduce, display, distribute, execute, and transmit the Software, 
* and to prepare derivative works of the Software, and to permit others
* to do so for non-commerical academic use, all subject to the following:
*
* The copyright notice in the Software and this entire statement, including 
* the above license grant, this restriction and the following disclaimer, 
* must be included in all copies of the Software, in whole or in part, and 
* all derivative works of the Software, unless such copies or derivative 
* works are solely in the form of machine-executable object code generated by 
* a source language processor.

* In any scientific publication based in part or wholly on the Software, the
* use of the Software has to be acknowledged and the publications quoted
* on the web page http://www.alps.org/license/ have to be referenced.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
* DEALINGS IN THE SOFTWARE.
*
**************************************************************************/

#ifndef ALPS_LATTICE_LATTICE_H
#define ALPS_LATTICE_LATTICE_H

#include <alps/config.h>
#include <alps/lattice/unitcell.h>
#include <alps/lattice/coordinate_traits.h>
#include <alps/vectorio.h>

namespace alps {

template <class L>
struct lattice_traits
{
};

template <class Lattice>
inline const typename lattice_traits<Lattice>::unit_cell_type&
unit_cell(const Lattice& l)
{
  return l.unit_cell();
}

template <class Lattice>
inline typename lattice_traits<Lattice>::cell_descriptor
cell(const typename lattice_traits<Lattice>::offset_type& o,const Lattice& l)
{
  return l.cell(o);
}

template <class Lattice>
inline const typename lattice_traits<Lattice>::offset_type&
offset(const typename lattice_traits<Lattice>::cell_descriptor& c, const Lattice& )
{
  return c.offset();
}

template <class Lattice>
inline typename lattice_traits<Lattice>::size_type
volume(const Lattice& l)
{
  return l.volume();
}

template <class Lattice>
inline bool 
on_lattice(typename lattice_traits<Lattice>::offset_type o, const Lattice& l)
{
  return l.on_lattice(o);
}

template <class Lattice>
inline std::pair<typename lattice_traits<Lattice>::cell_iterator,
                 typename lattice_traits<Lattice>::cell_iterator>
cells(const Lattice& l)
{
  return l.cells();
}

template <class Lattice>
inline std::pair<bool, typename lattice_traits<Lattice>::boundary_crossing_type>
shift(typename lattice_traits<Lattice>::offset_type& o,
      const typename lattice_traits<Lattice>::offset_type& s,
      const Lattice& l)
{
  return l.shift(o,s);
}

template <class Lattice>
inline typename lattice_traits<Lattice>::size_type
index(typename lattice_traits<Lattice>::cell_descriptor c, const Lattice& l)
{
  return l.index(c);
}

template <class Lattice>
inline std::pair<typename lattice_traits<Lattice>::basis_vector_iterator,
                 typename lattice_traits<Lattice>::basis_vector_iterator>
basis_vectors(const Lattice& l)
{
  return l.basis_vectors();
}

template <class Lattice>
inline std::pair<typename lattice_traits<Lattice>::basis_vector_iterator,
                 typename lattice_traits<Lattice>::basis_vector_iterator>
reciprocal_basis_vectors(const Lattice& l)
{
  return l.reciprocal_basis_vectors();
}

template <class Lattice>
inline typename lattice_traits<Lattice>::vector_type
coordinate(const typename lattice_traits<Lattice>::cell_descriptor& c, 
       const typename lattice_traits<Lattice>::vector_type& p, const Lattice& l)
{
  typename lattice_traits<Lattice>::basis_vector_iterator first, last;
  typedef typename coordinate_traits<typename lattice_traits<Lattice>::offset_type
    >::const_iterator offset_iterator;
  boost::tie(first,last) = basis_vectors(l);
  offset_iterator off = coordinates(offset(c,l)).first; 
  typename lattice_traits<Lattice>::vector_type v(alps::dimension(*first));
  for (int i=0; first!=last; ++first, ++off,++i)
    v = v + (*first) * ((*off)+(p.size() ? p[i] : 0));
  return v;
}
    
template <class Lattice>
inline typename lattice_traits<Lattice>::vector_type
origin(const typename lattice_traits<Lattice>::cell_descriptor& c, const Lattice& l)
{
  typename lattice_traits<Lattice>::basis_vector_iterator first, last;
  typedef typename coordinate_traits<typename lattice_traits<Lattice>::offset_type
    >::const_iterator offset_iterator;
  boost::tie(first,last) = basis_vectors(l);
  offset_iterator off = coordinates(offset(c,l)).first; 
  if (first!=last) {
    typename lattice_traits<Lattice>::vector_type v(*first);
    v*=*off;
    ++first;
    ++off;
    for (; first!=last; ++first, ++off)
      v = v + (*first) * (*off);
    return v;
  }
  else
    return lattice_traits<Lattice>::vector_type();
}

void prevent_optimization();

template <class Lattice>
inline std::pair<typename lattice_traits<Lattice>::momentum_iterator,
                 typename lattice_traits<Lattice>::momentum_iterator>
momenta(const Lattice& l)
{
  return l.momenta();
}

template <class Lattice>
inline typename lattice_traits<Lattice>::vector_type
momentum(const typename lattice_traits<Lattice>::vector_type& m, const Lattice& l)
{
  typename lattice_traits<Lattice>::basis_vector_iterator first, last;
  boost::tie(first,last) = reciprocal_basis_vectors(l);
  if (first!=last) {
    typename lattice_traits<Lattice>::vector_type v(*first);
    v*=m[0]/(2.*MP_PI);
    ++first;
    for (int i=1; first!=last; ++first, ++i)
      v = v + (*first) * m[i]/(2.*MP_PI);
    return v;
  }
  else
    return m;
}

} // end namespace alps

#endif // ALPS_LATTICE_LATTICE_H
