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
        // This wrapping apply is optional, but both with and without it the
        // behaviour is slightly wrong. Without the wrapping apply dataflow
        // becomes blocking with ready arguments to dataflow. With the wrapping
        // apply the future is set to ready too early.
        hpx::apply([f = std::move(f), t = std::move(t)]() {
            // This sets the dataflow future to ready immediately when the
            // user-provided function returns. There is no place to wait for the
            // completion of the underlying operation.
            hpx::invoke(f, std::move(t));
            std::cout << "dataflow finalization: asynchronous operation "
                         "spawned and dataflow future set to ready\n"
                      << std::flush;

            // We can wait for the completion here, but it is of little use.
            hpx::util::yield_while([]() { return !done; });
            std::cout << "dataflow finalization: asynchronous operation done\n"
                      << std::flush;
        });
    }
};

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
