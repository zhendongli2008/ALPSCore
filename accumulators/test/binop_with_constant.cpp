/*
 * Copyright (C) 1998-2015 ALPS Collaboration. See COPYRIGHT.TXT
 * All rights reserved. Use is subject to license terms. See LICENSE.TXT
 * For use in publications, see ACKNOWLEDGE.TXT
 */

/** @file binop_with_constant.cpp
    Test simple binary operations between results and constants.
*/

/*
  Things to test:
  1. 4 binary ops: +=, -=, *=, /=
  2. Various features: Mean, NoBinning, LogBinning, FullBinning
  2. LHS types: [scalar, vector] x [float, double, long double]
  3. RHS types.

  This can be done using Google typed tests, but on members of
  Cartesian product of tested types. There seems to be no way to run
  Google Test on Cartesian product of types or on MPL vector. So, we
  have to generate the source manually.
*/

#include <cmath>
#include "gtest/gtest.h"

#include "alps/accumulators.hpp"

/// Functions to compare vector or scalar values
/// Compare vector values
template <typename T>
inline void compare_near(const std::vector<T>& expected, const std::vector<T>& actual, double tol, const std::string& descr)
{
    EXPECT_EQ(expected.size(), actual.size()) << "Sizes of "+descr+" differ";
    for (int i=0; i<expected.size(); ++i) {
        EXPECT_NEAR(expected.at(i), actual.at(i), tol) << "Element #"+boost::lexical_cast<std::string>(i)+" of "+descr+" differs";
    }
}

/// Compare scalar values
template <typename T>
inline void compare_near(T expected, T actual, double tol, const std::string& descr)
{
    EXPECT_NEAR(expected, actual, tol) << "Values of "+descr+" differ";
}

/// Get scalar data point 
template <typename T>
inline T get_data(T val) { return val; }

// Get vector data point
template <typename T>
inline T get_data(typename T::value_type val)
{
    return T(10, val);
}


// Class to generate accumulators and results of a given type
template <typename A>
class AccResultGenerator  {
  private:
    alps::accumulators::result_set* results_ptr_;
    alps::accumulators::accumulator_set* measurements_ptr_;
    const std::string name_;
    
  public:
    typedef A named_acc_type;
    typedef typename alps::accumulators::value_type<typename named_acc_type::accumulator_type>::type value_type;

    static const unsigned long int NPOINTS=10000; /// < Number of data points
    static double tol() { return 5.E-3; }         /// < Recommended tolerance to compare expected and actual results
    /// Free the memory allocated in the constructor
    virtual ~AccResultGenerator()
    {
        delete results_ptr_;
        delete measurements_ptr_;
    }
    
    /// Generate the data points for the accumulator
    AccResultGenerator() : name_("acc")
    {
        srand48(43);
        measurements_ptr_=new alps::accumulators::accumulator_set();
        alps::accumulators::accumulator_set& m=*measurements_ptr_;
        m << named_acc_type(name_);
        for (int i=0; i<NPOINTS; ++i) {
            double d=drand48();
            m[name_] << get_data<value_type>(d);
        }
        results_ptr_=new alps::accumulators::result_set(m);
    }

    /// Returns extracted results
    const alps::accumulators::result_wrapper& result() const
    {
        return (*results_ptr_)[name_];
    }

    /// Returns the underlying accumulator
    const alps::accumulators::accumulator_wrapper& accumulator() const
    {
        return (*measurements_ptr_)[name_];
    }

    /// Returns the expected mean
    double expected_mean() const { return 0.5; }
    /// Returns the expected error
    double expected_err() const { return 1/(12*sqrt(NPOINTS-1.)); }
};


/// Google Test Fixture: argument is std::pair<A1,T2> to test A1() $ T2() (where $ is +,-,*,/)
template <typename TPair>
class AccumulatorBinaryOpTest : public ::testing::Test {
  public:
    typedef typename TPair::first_type lhs_acc_type;
    typedef AccResultGenerator<lhs_acc_type> acc_gen_type;
    typedef typename acc_gen_type::value_type lhs_data_type;
    typedef typename TPair::second_type rhs_data_type;

  private:
    // FIXME: The accumulator is a bit expensive to generate, but a static
    // variable is a pain to declare across 27 template classes. So,
    // let it be generated for every test. :(
    acc_gen_type acc_gen_;

  public:
    /// Test addition
    void add()  const
    {
        const alps::accumulators::result_wrapper& res=acc_gen_.result() + get_data<rhs_data_type>(2.0);
        compare_near(get_data<lhs_data_type>(acc_gen_.expected_mean()+2.0), res.mean<lhs_data_type>(), acc_gen_type::tol(), "Mean value");
        compare_near(get_data<lhs_data_type>(acc_gen_.expected_err()), res.error<lhs_data_type>(), acc_gen_type::tol(), "Error value");
    }
    
    /// Test subtraction
    void sub()  const
    {
        const alps::accumulators::result_wrapper& res=acc_gen_.result() - get_data<rhs_data_type>(2.0);
        compare_near(get_data<lhs_data_type>(acc_gen_.expected_mean()-2.0), res.mean<lhs_data_type>(), acc_gen_type::tol(), "Mean value");
        compare_near(get_data<lhs_data_type>(acc_gen_.expected_err()), res.error<lhs_data_type>(), acc_gen_type::tol(), "Error value");
    }
    
    /// Test multiplication
    void mul()  const
    {
        const alps::accumulators::result_wrapper& res=acc_gen_.result() * get_data<rhs_data_type>(2.0);
        compare_near(get_data<lhs_data_type>(acc_gen_.expected_mean()*2.0), res.mean<lhs_data_type>(), acc_gen_type::tol(), "Mean value");
        compare_near(get_data<lhs_data_type>(acc_gen_.expected_err()*2.0), res.error<lhs_data_type>(), acc_gen_type::tol(), "Error value");
    }
    
    /// Test division
    void div()  const
    {
        const alps::accumulators::result_wrapper& res=acc_gen_.result() / get_data<rhs_data_type>(2.0);
        compare_near(get_data<lhs_data_type>(acc_gen_.expected_mean()/2.0), res.mean<lhs_data_type>(), acc_gen_type::tol(), "Mean value");
        compare_near(get_data<lhs_data_type>(acc_gen_.expected_err()/2.0), res.error<lhs_data_type>(), acc_gen_type::tol(), "Error value");
    }
};

TYPED_TEST_CASE_P(AccumulatorBinaryOpTest);

TYPED_TEST_P(AccumulatorBinaryOpTest,add) { this->TestFixture::add(); }
TYPED_TEST_P(AccumulatorBinaryOpTest,sub) { this->TestFixture::sub(); }
TYPED_TEST_P(AccumulatorBinaryOpTest,mul) { this->TestFixture::mul(); }
TYPED_TEST_P(AccumulatorBinaryOpTest,div) { this->TestFixture::div(); }

REGISTER_TYPED_TEST_CASE_P(AccumulatorBinaryOpTest, add, sub, mul, div);

typedef long double long_double;
typedef std::vector<float> float_vec;
typedef std::vector<double> double_vec;
typedef std::vector<long_double> long_double_vec;

#define ALPS_TEST_SCALARS_SEQ (float)(double)(long_double)
#define ALPS_TEST_VECTORS_SEQ (float_vec)(double_vec)(long_double_vec)
#define ALPS_TEST_ANAME_SEQ (NoBinningAccumulator)(LogBinningAccumulator)(FullBinningAccumulator)

// Now, generate all possible pairs of typed named accumulators with RHS types.
// Unfortunately, BOOST_PP seems to be unable to do it (or at least not in a easy way).
// Therefore, we resolve to using shell :) and generate all pairs explicitly.

// Shell template string: "std::pair<alps::accumulators::${acc}BinningAccumulator<${lht}>,${rht}>,"
// Shell loop: for acc in $accs; do for lht in $lhts; do for rht in $rhts; do echo .....; done; done; done

// Scalar-scalar pairs: accs="No Log Full"; lhts="float double long_double"; rhts="float double long_double"

typedef ::testing::Types<
    std::pair<alps::accumulators::NoBinningAccumulator<float>,float>,
    std::pair<alps::accumulators::NoBinningAccumulator<float>,double>,
    std::pair<alps::accumulators::NoBinningAccumulator<float>,long_double>,
    std::pair<alps::accumulators::NoBinningAccumulator<double>,float>,
    std::pair<alps::accumulators::NoBinningAccumulator<double>,double>,
    std::pair<alps::accumulators::NoBinningAccumulator<double>,long_double>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double>,float>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double>,double>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double>,long_double>,
    std::pair<alps::accumulators::LogBinningAccumulator<float>,float>,
    std::pair<alps::accumulators::LogBinningAccumulator<float>,double>,
    std::pair<alps::accumulators::LogBinningAccumulator<float>,long_double>,
    std::pair<alps::accumulators::LogBinningAccumulator<double>,float>,
    std::pair<alps::accumulators::LogBinningAccumulator<double>,double>,
    std::pair<alps::accumulators::LogBinningAccumulator<double>,long_double>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double>,float>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double>,double>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double>,long_double>,
    std::pair<alps::accumulators::FullBinningAccumulator<float>,float>,
    std::pair<alps::accumulators::FullBinningAccumulator<float>,double>,
    std::pair<alps::accumulators::FullBinningAccumulator<float>,long_double>,
    std::pair<alps::accumulators::FullBinningAccumulator<double>,float>,
    std::pair<alps::accumulators::FullBinningAccumulator<double>,double>,
    std::pair<alps::accumulators::FullBinningAccumulator<double>,long_double>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double>,float>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double>,double>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double>,long_double>
   > Scalar_Scalar_types;

INSTANTIATE_TYPED_TEST_CASE_P(ScalarScalar, AccumulatorBinaryOpTest, Scalar_Scalar_types);

// Vector-scalar pairs: accs="No Log Full"; lhts="float_vec double_vec long_double_vec"; rhts="float double long_double"
typedef ::testing::Types<
    std::pair<alps::accumulators::NoBinningAccumulator<float_vec>,float>,
    std::pair<alps::accumulators::NoBinningAccumulator<float_vec>,double>,
    std::pair<alps::accumulators::NoBinningAccumulator<float_vec>,long_double>,
    std::pair<alps::accumulators::NoBinningAccumulator<double_vec>,float>,
    std::pair<alps::accumulators::NoBinningAccumulator<double_vec>,double>,
    std::pair<alps::accumulators::NoBinningAccumulator<double_vec>,long_double>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double_vec>,float>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double_vec>,double>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double_vec>,long_double>,
    std::pair<alps::accumulators::LogBinningAccumulator<float_vec>,float>,
    std::pair<alps::accumulators::LogBinningAccumulator<float_vec>,double>,
    std::pair<alps::accumulators::LogBinningAccumulator<float_vec>,long_double>,
    std::pair<alps::accumulators::LogBinningAccumulator<double_vec>,float>,
    std::pair<alps::accumulators::LogBinningAccumulator<double_vec>,double>,
    std::pair<alps::accumulators::LogBinningAccumulator<double_vec>,long_double>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double_vec>,float>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double_vec>,double>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double_vec>,long_double>,
    std::pair<alps::accumulators::FullBinningAccumulator<float_vec>,float>,
    std::pair<alps::accumulators::FullBinningAccumulator<float_vec>,double>,
    std::pair<alps::accumulators::FullBinningAccumulator<float_vec>,long_double>,
    std::pair<alps::accumulators::FullBinningAccumulator<double_vec>,float>,
    std::pair<alps::accumulators::FullBinningAccumulator<double_vec>,double>,
    std::pair<alps::accumulators::FullBinningAccumulator<double_vec>,long_double>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double_vec>,float>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double_vec>,double>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double_vec>,long_double>
    > Vector_Scalar_types;

INSTANTIATE_TYPED_TEST_CASE_P(VectorScalar, AccumulatorBinaryOpTest, Vector_Scalar_types);

// Vector-Vector pairs: accs="No Log Full"; lhts="float_vec double_vec long_double_vec"; rhts="float_vec double_vec long_double_vec"
// FIXME: this does not really work!
typedef ::testing::Types<
    std::pair<alps::accumulators::NoBinningAccumulator<float_vec>,float_vec>,
    std::pair<alps::accumulators::NoBinningAccumulator<float_vec>,double_vec>,
    std::pair<alps::accumulators::NoBinningAccumulator<float_vec>,long_double_vec>,
    std::pair<alps::accumulators::NoBinningAccumulator<double_vec>,float_vec>,
    std::pair<alps::accumulators::NoBinningAccumulator<double_vec>,double_vec>,
    std::pair<alps::accumulators::NoBinningAccumulator<double_vec>,long_double_vec>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double_vec>,float_vec>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double_vec>,double_vec>,
    std::pair<alps::accumulators::NoBinningAccumulator<long_double_vec>,long_double_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<float_vec>,float_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<float_vec>,double_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<float_vec>,long_double_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<double_vec>,float_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<double_vec>,double_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<double_vec>,long_double_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double_vec>,float_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double_vec>,double_vec>,
    std::pair<alps::accumulators::LogBinningAccumulator<long_double_vec>,long_double_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<float_vec>,float_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<float_vec>,double_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<float_vec>,long_double_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<double_vec>,float_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<double_vec>,double_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<double_vec>,long_double_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double_vec>,float_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double_vec>,double_vec>,
    std::pair<alps::accumulators::FullBinningAccumulator<long_double_vec>,long_double_vec>
 > Vector_Vector_types;

// Produces compilation error:
// INSTANTIATE_TYPED_TEST_CASE_P(VectorVector, AccumulatorBinaryOpTest, Vector_Vector_types);
