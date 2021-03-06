/*
 * Copyright (C) 1998-2018 ALPS Collaboration. See COPYRIGHT.TXT
 * All rights reserved. Use is subject to license terms. See LICENSE.TXT
 * For use in publications, see ACKNOWLEDGE.TXT
 */
#include"alps/gf/gf.hpp"

namespace alps{
    namespace gf{

        void save_version(alps::hdf5::archive& ar, const std::string& path)
        {
            std::string vp=path+"/version/";
            ar[vp+"minor"]<< int(minor_version);
            ar[vp+"major"]<< int(major_version);
            ar[vp+"reference"]<<"https://github.com/ALPSCore/H5GF/blob/master/H5GF.rst";
            ar[vp+"originator"]<<"ALPSCore GF library, see http://www.alpscore.org";
        }

        bool check_version(alps::hdf5::archive& ar, const std::string& path)
        {
            std::string vp=path+"/version/";
            int ver;
            ar[vp+"major"]>>ver;
            return (major_version==ver);
        }

        namespace detail{
            template<> void print_no_complex(std::ostream &os, const std::complex<double> &z){
                os<<z.real()<<" "<<z.imag();
            }

        }
    }
}
