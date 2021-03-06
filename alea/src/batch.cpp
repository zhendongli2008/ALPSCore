#include <alps/alea/batch.hpp>
#include <alps/alea/variance.hpp>
#include <alps/alea/covariance.hpp>
#include <alps/alea/internal/util.hpp>

#include <numeric>

namespace alps { namespace alea {

template <typename T>
batch_data<T>::batch_data(size_t size, size_t num_batches)
    : batch_(size, num_batches)
    , count_(num_batches)
{
    reset();
}

template <typename T>
void batch_data<T>::reset()
{
    batch_.fill(0);
    count_.fill(0);
}

template class batch_data<double>;
template class batch_data<std::complex<double> >;


template <typename T>
batch_acc<T>::batch_acc(size_t size, size_t num_batches, size_t base_size)
    : size_(size)
    , num_batches_(num_batches)
    , base_size_(base_size)
    , store_(new batch_data<T>(size, num_batches))
    , cursor_(num_batches)
    , offset_(num_batches)
{
    if (num_batches % 2 != 0) {
        throw std::runtime_error("Number of batches must be even to allow "
                                 "for rebatching.");
    }
    for (size_t i = 0; i != num_batches; ++i)
        offset_[i] = i * base_size_;
}

template <typename T>
batch_acc<T>::batch_acc(const batch_acc &other)
    : size_(other.size_)
    , num_batches_(other.num_batches_)
    , base_size_(other.base_size_)
    , store_(other.store_ ? new batch_data<T>(*other.store_) : nullptr)
    , cursor_(other.cursor_)
    , offset_(other.offset_)
{ }

template <typename T>
batch_acc<T> &batch_acc<T>::operator=(const batch_acc &other)
{
    size_ = other.size_;
    num_batches_ = other.num_batches_;
    base_size_ = other.base_size_;
    store_.reset(other.store_ ? new batch_data<T>(*other.store_) : nullptr);
    cursor_ = other.cursor_;
    offset_ = other.offset_;
    return *this;
}

template <typename T>
void batch_acc<T>::reset()
{
    cursor_.reset();
    for (size_t i = 0; i != num_batches_; ++i)
        offset_[i] = i * base_size_;

    if (valid())
        store_->reset();
    else
        store_.reset(new batch_data<T>(size_, num_batches_));
}

template <typename T>
void batch_acc<T>::add(const computed<T> &source, size_t count)
{
    internal::check_valid(*this);

    // batch is full, move the cursor.
    // Doing this before the addition ensures no empty batches.
    if (store_->count()(cursor_.current()) >= current_batch_size())
        next_batch();

    // Since Eigen matrix are column-major, we can just pass the pointer
    source.add_to(sink<T>(store_->batch().col(cursor_.current()).data(), size()));
    store_->count()(cursor_.current()) += count;
}

template <typename T>
void batch_acc<T>::next_batch()
{
    ++cursor_;
    if (cursor_.merge_mode()) {
        // merge counts
        store_->count()(cursor_.merge_into()) += store_->count()(cursor_.current());
        store_->count()(cursor_.current()) = 0;

        // merge batches
        store_->batch().col(cursor_.merge_into()) +=
                                        store_->batch().col(cursor_.current());
        store_->batch().col(cursor_.current()).fill(0);

        // merge offsets
        offset_(cursor_.merge_into()) = std::min(offset_(cursor_.merge_into()),
                                                 offset_(cursor_.current()));
        offset_(cursor_.current()) = count();
    }
}

template <typename T>
batch_result<T> batch_acc<T>::result() const
{
    internal::check_valid(*this);
    batch_result<T> result(*store_);
    return result;
}

template <typename T>
batch_result<T> batch_acc<T>::finalize()
{
    batch_result<T> result;
    finalize_to(result);
    return result;
}

template <typename T>
void batch_acc<T>::finalize_to(batch_result<T> &result)
{
    internal::check_valid(*this);
    result.store_.reset();
    result.store_.swap(store_);
}

template class batch_acc<double>;
template class batch_acc<std::complex<double> >;


template <typename T>
batch_result<T>::batch_result(const batch_result &other)
    : store_(other.store_ ? new batch_data<T>(*other.store_) : nullptr)
{ }

template <typename T>
batch_result<T> &batch_result<T>::operator=(const batch_result &other)
{
    store_.reset(other.store_ ? new batch_data<T>(*other.store_) : nullptr);
    return *this;
}

template <typename T>
column<T> batch_result<T>::mean() const
{
    return store_->batch().rowwise().sum() / count();
}

template <typename T>
template <typename Str>
column<typename bind<Str,T>::var_type> batch_result<T>::var() const
{
    var_acc<T, Str> aux_acc(store_->size());
    for (size_t i = 0; i != store_->num_batches(); ++i) {
        aux_acc.add(make_adapter(store_->batch().col(i)), store_->count()(i),
                    nullptr);
    }
    return aux_acc.finalize().var();
}

template <typename T>
template <typename Str>
column<typename bind<Str,T>::cov_type> batch_result<T>::cov() const
{
    cov_acc<T, Str> aux_acc(store_->size());
    for (size_t i = 0; i != store_->num_batches(); ++i)
        aux_acc.add(make_adapter(store_->batch().col(i)), store_->count()(i));
    return aux_acc.finalize().cov();
}

template <typename T>
column<typename bind<circular_var,T>::var_type> batch_result<T>::stderror() const
{
    var_acc<T, circular_var> aux_acc(store_->size());
    for (size_t i = 0; i != store_->num_batches(); ++i) {
        aux_acc.add(make_adapter(store_->batch().col(i)), store_->count()(i),
                    nullptr);
    }
    return aux_acc.finalize().stderror();
}

template <typename T>
void batch_result<T>::reduce(const reducer &r, bool pre_commit, bool post_commit)
{
    // FIXME this is bad since it mixes bins
    internal::check_valid(*this);
    if (pre_commit) {
        r.reduce(sink<T>(store_->batch().data(), store_->batch().size()));
        r.reduce(sink<size_t>(store_->count().data(), store_->num_batches()));
    }
    if (pre_commit && post_commit) {
        r.commit();
    }
    if (post_commit) {
        reducer_setup setup = r.get_setup();
        if (!setup.have_result)
            store_.reset();   // free data
    }
}

template <typename T>
void batch_result<T>::serialize(serializer &s) const
{
    internal::check_valid(*this);
    s.write("count", make_adapter(count()));
    s.write("mean/value", make_adapter(mean()));
    s.write("mean/error", make_adapter(stderror()));

    // FIXME
    typename eigen<T>::col_map batch_map(store_->batch().data(), store_->batch().size());
    s.write("batch/count", make_adapter(store_->count().transpose()));
    s.write("batch/sum", make_adapter(batch_map));
}

template column<double> batch_result<double>::var<circular_var>() const;
template column<double> batch_result<std::complex<double> >::var<circular_var>() const;
template column<complex_op<double> > batch_result<std::complex<double> >::var<elliptic_var>() const;

template column<double> batch_result<double>::cov< circular_var>() const;
template column<std::complex<double> > batch_result<std::complex<double> >::cov<circular_var>() const;
template column<complex_op<double> > batch_result<std::complex<double> >::cov<elliptic_var>() const;

template class batch_result<double>;
template class batch_result<std::complex<double> >;

}}
