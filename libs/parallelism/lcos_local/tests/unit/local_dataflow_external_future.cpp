//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2015 Hartmut Kaiser
//  Copyright (c) 2013 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/future.hpp>
#include <hpx/program_options.hpp>
#include <hpx/wrap_main.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

std::atomic<bool> done{false};

///////////////////////////////////////////////////////////////////////////////
// Note that this custom future_data is not strictly required for out-of-band
// signaling.
template <typename R, typename Allocator = hpx::util::internal_allocator<>>
struct custom_future_data
  : hpx::lcos::detail::future_data_allocator<R, Allocator>
{
    HPX_NON_COPYABLE(custom_future_data);

    using init_no_addref =
        typename hpx::lcos::detail::future_data_allocator<void,
            Allocator>::init_no_addref;

    using other_allocator = typename std::allocator_traits<
        Allocator>::template rebind_alloc<custom_future_data>;

    custom_future_data() {}

    custom_future_data(init_no_addref no_addref)
      : hpx::lcos::detail::future_data_allocator<void, Allocator>(
            no_addref, Allocator{})
    {
    }

    custom_future_data(init_no_addref no_addref, other_allocator const& alloc)
      : hpx::lcos::detail::future_data_allocator<void, Allocator>(
            no_addref, alloc)
    {
    }
};

struct external_future_executor
{
    // This is not actually called by dataflow, but it is used for the return
    // type calculation of it.
    template <typename F, typename... Ts>
    decltype(auto) async_execute(F&& f, Ts&&... ts)
    {
        // The completion of f is signalled out-of-band.
        hpx::invoke(std::forward<F>(f), std::forward<Ts>(ts)...);
        return hpx::async(
            []() { hpx::util::yield_while([]() { return !done; }); });
    }

    // This overload is used for dataflow finalization.
    template <typename A, typename... Ts>
    void post(
        hpx::lcos::detail::dataflow_finalization<A>&& f, hpx::tuple<Ts...>&& t)
    {
        std::cout << "dataflow finalization: calling post\n" << std::flush;
        hpx::invoke(f, std::move(t));
        std::cout << "dataflow finalization: called post\n" << std::flush;
    }
};

namespace hpx { namespace lcos { namespace detail {
    template <typename Func, typename Futures>
    struct dataflow_future_base<external_future_executor, Func, Futures>
    {
        // NOTE: Customizing this is not strictly necessary. external_
        //using future_base_type =
        //    hpx::lcos::detail::future_data<typename hpx::traits::future_traits<
        //        typename detail::dataflow_return<external_future_executor, Func,
        //            Futures>::type>::type>;
        using future_base_type =
            custom_future_data<typename hpx::traits::future_traits<
                typename detail::dataflow_return<external_future_executor, Func,
                    Futures>::type>::type>;

        template <typename Frame, typename Futures_>
        static void execute(Frame&& frame,
            typename std::decay<Func>::type&& func, std::false_type,
            Futures_&& futures)
        {
            std::exception_ptr p;

            try
            {
                auto&& r = util::invoke_fused(
                    std::forward<Func>(func), std::forward<Futures_>(futures));

                // Signal completion from another thread/task.
                hpx::intrusive_ptr<typename std::remove_pointer<
                    typename std::decay<Frame>::type>::type>
                    frame_p(frame);
                hpx::apply([frame_p = std::move(frame_p), r = std::move(r)]() {
                    hpx::util::yield_while([]() { return !done; });
                    frame_p->set_data(std::move(r));
                });
                return;
            }
            catch (...)
            {
                p = std::current_exception();
            }

            // The exception is set outside the catch block since
            // set_exception may yield. Ending the catch block on a
            // different worker thread than where it was started may lead
            // to segfaults.
            frame->set_exception(std::move(p));
        }

        template <typename Frame, typename Futures_>
        static void execute(Frame&& frame,
            typename std::decay<Func>::type&& func, std::true_type,
            Futures_&& futures)
        {
            std::exception_ptr p;

            try
            {
                util::invoke_fused(
                    std::forward<Func>(func), std::forward<Futures_>(futures));

                // Signal completion from another thread/task.
                hpx::intrusive_ptr<typename std::remove_pointer<
                    typename std::decay<Frame>::type>::type>
                    frame_p(frame);
                hpx::apply([frame_p = std::move(frame_p)]() {
                    hpx::util::yield_while([]() { return !done; });
                    frame_p->set_data(util::unused_type());
                });
                return;
            }
            catch (...)
            {
                p = std::current_exception();
            }

            // The exception is set outside the catch block since
            // set_exception may yield. Ending the catch block on a
            // different worker thread than where it was started may lead
            // to segfaults.
            frame->set_exception(std::move(p));
        }
    };
}}}    // namespace hpx::lcos::detail

namespace hpx { namespace parallel { namespace execution {
    template <>
    struct is_one_way_executor<external_future_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<external_future_executor> : std::true_type
    {
    };
}}}    // namespace hpx::parallel::execution

int main()
{
    {
        external_future_executor exec;
        hpx::future<void> f = hpx::dataflow(exec, []() {
            // This represents an asynchronous operation which has an
            // out-of-band mechanism for signaling completion.
            hpx::apply([]() {
                std::cout << "asynchronous operation started\n" << std::flush;
                hpx::this_thread::sleep_for(std::chrono::seconds(3));
                std::cout << "asynchronous operation ready\n" << std::flush;
                done = true;
            });
        });

        std::cout << "waiting for future\n" << std::flush;
        f.get();
        std::cout << "future is ready\n" << std::flush;
    }

    return 0;
}
