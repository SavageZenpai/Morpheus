/*
 * SPDX-FileCopyrightText: Copyright (c) 2021-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "morpheus/stages/deserialize.hpp"

#include "morpheus/messages/control.hpp"
#include "morpheus/types.hpp"

#include <pybind11/pybind11.h>
#include <pymrc/utils.hpp>  // for cast_from_pyobject
// IWYU pragma: no_include "rxcpp/sources/rx-iterate.hpp"

namespace morpheus {


void make_output_message(std::shared_ptr<MessageMeta>& incoming_message,
                         TensorIndex start,
                         TensorIndex stop,
                         cm_task_t* task,
                         std::shared_ptr<ControlMessage>& windowed_message)
{
    auto slidced_meta = std::make_shared<SlicedMessageMeta>(incoming_message, start, stop);
    auto message      = std::make_shared<ControlMessage>();
    message->payload(slidced_meta);
    if (task)
    {
        message->add_task(task->first, task->second);
    }

    windowed_message.swap(message);
}

DeserializeStage::subscribe_fn_t DeserializeStage::build_operator()
{
    return [this](rxcpp::observable<sink_type_t> input, rxcpp::subscriber<source_type_t> output) {
        return input.subscribe(rxcpp::make_observer<sink_type_t>(
            [this, &output](sink_type_t incoming_message) {
                if (!incoming_message->has_sliceable_index())
                {
                    if (m_ensure_sliceable_index)
                    {
                        auto old_index_name = incoming_message->ensure_sliceable_index();

                        if (old_index_name.has_value())
                        {
                            // Generate a warning
                            LOG(WARNING) << MORPHEUS_CONCAT_STR(
                                "Incoming MessageMeta does not have a unique and monotonic index. Updating index "
                                "to be unique. Existing index will be retained in column '"
                                << *old_index_name << "'");
                        }
                    }
                    else
                    {
                        utilities::show_warning_message(
                            "Detected a non-sliceable index on an incoming MessageMeta. Performance when taking slices "
                            "of messages may be degraded. Consider setting `ensure_sliceable_index==True`",
                            PyExc_RuntimeWarning);
                    }
                }
                // Loop over the MessageMeta and create sub-batches
                for (TensorIndex i = 0; i < incoming_message->count(); i += this->m_batch_size)
                {
                    std::shared_ptr<ControlMessage> windowed_message{nullptr};
                    make_output_message(incoming_message,
                                        i,
                                        std::min(i + this->m_batch_size, incoming_message->count()),
                                        m_task.get(),
                                        windowed_message);
                    output.on_next(std::move(windowed_message));
                }
            },
            [&](std::exception_ptr error_ptr) {
                output.on_error(error_ptr);
            },
            [&]() {
                output.on_completed();
            }));
    };
}


std::shared_ptr<mrc::segment::Object<DeserializeStage>> DeserializeStageInterfaceProxy::init(
    mrc::segment::Builder& builder,
    const std::string& name,
    TensorIndex batch_size,
    bool ensure_sliceable_index,
    const pybind11::object& task_type,
    const pybind11::object& task_payload)
{
    std::unique_ptr<cm_task_t> task{nullptr};

    if (!task_type.is_none() && !task_payload.is_none())
    {
        task = std::make_unique<cm_task_t>(pybind11::cast<std::string>(task_type),
                                           mrc::pymrc::cast_from_pyobject(task_payload));
    }

    auto stage = builder.construct_object<DeserializeStage>(
        name, batch_size, ensure_sliceable_index, std::move(task));

    return stage;
}

}  // namespace morpheus
