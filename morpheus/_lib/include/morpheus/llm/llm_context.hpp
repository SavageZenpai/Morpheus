/*
 * SPDX-FileCopyrightText: Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#pragma once

#include "morpheus/export.h"
#include "morpheus/llm/fwd.hpp"  // for ControlMessage
#include "morpheus/llm/input_map.hpp"
#include "morpheus/llm/llm_task.hpp"

#include <mrc/types.hpp>
#include <pymrc/utilities/json_values.hpp>

#include <memory>
#include <string>
#include <vector>

// IWYU mistakenly believes that we could use the forward declares of LLMTask and ControlMessage in fwd.hpp, however an
// an incomplete type decl cannot be used in a shared_ptr, and in the case of LLMTask in a struct that is used in a
// shared_ptr.
// IWYU pragma: no_include "morpheus/llm/fwd.hpp"

namespace morpheus::llm {

struct LLMContextState
{
    LLMTask task;
    std::shared_ptr<ControlMessage> message;

    // Optional row mask to be applied to the Dataframe by the extractor and task handler to filter rows
    std::vector<bool> row_mask;
};

/**
 * @brief Holds and manages information related to LLM tasks and input mappings required for LLMNode execution.
 * Outputs of node executions are also saved here for use by nodes and task handlers in LLMEngine.
 */
class MORPHEUS_EXPORT LLMContext : public std::enable_shared_from_this<LLMContext>
{
  public:
    /**
     * @brief Construct a new LLMContext object.
     *
     */
    LLMContext();

    /**
     * @brief Construct a new LLMContext object.
     *
     * @param task task for new context
     * @param message control message for new context
     */
    LLMContext(LLMTask task, std::shared_ptr<ControlMessage> message);

    /**
     * @brief Construct a new LLMContext object.
     *
     * @param parent parent context
     * @param name new context name
     * @param inputs input mappings for new context
     * @param row_mask row mask for new context
     */
    LLMContext(std::shared_ptr<LLMContext> parent, std::string name, input_mappings_t inputs);

    /**
     * @brief Destroy the LLMContext object.
     */
    ~LLMContext();

    /**
     * @brief Get parent context.
     *
     * @return std::shared_ptr<LLMContext>
     */
    std::shared_ptr<LLMContext> parent() const;

    /**
     * @brief Get name of context.
     *
     * @return const std::string&
     */
    const std::string& name() const;

    /**
     * @brief Get map of internal mappings for this context.
     *
     * @return const input_mappings_t&
     */
    const input_mappings_t& input_map() const;

    /**
     * @brief Get task for this context.
     *
     * @return const LLMTask&
     */
    const LLMTask& task() const;

    /**
     * @brief Get control message for this context.
     *
     * @return std::shared_ptr<ControlMessage>&
     */
    std::shared_ptr<ControlMessage>& message() const;

    /**
     * @brief Get all outputs for this context.
     *
     * @return const mrc::pymrc::JSONValues&
     */
    const mrc::pymrc::JSONValues& all_outputs() const;

    /**
     * @brief Get full name of context containing parents up to root.
     *
     * @return std::string
     */
    std::string full_name() const;

    /**
     * @brief Create new context from this context using provided name and input mappings.
     *
     * @param name name of new context
     * @param inputs input mappings for new context
     * @return std::shared_ptr<LLMContext>
     */
    std::shared_ptr<LLMContext> push(std::string name, input_mappings_t inputs);

    /**
     * @brief Moves output map from this context to parent context. Outputs to move can be selected using
     * set_output_names, otherwise all outputs are noved by default.
     *
     */
    void pop();

    /**
     * @brief Get the input value from parent context corresponding to first internal input of this context.
     *
     * @return mrc::pymrc::JSONValues
     */
    mrc::pymrc::JSONValues get_input() const;

    /**
     * @brief Get the parent output value corresponding to given internal input name.
     *
     * @param node_name internal input name
     * @return mrc::pymrc::JSONValues
     */
    mrc::pymrc::JSONValues get_input(const std::string& node_name) const;

    /**
     * @brief Get parent output values corresponding to all internal input names.
     *
     * @return mrc::pymrc::JSONValues
     */
    mrc::pymrc::JSONValues get_inputs() const;

    /**
     * @brief Set output mappings for this context.
     *
     * @param outputs output mappings
     */
    void set_output(mrc::pymrc::JSONValues&& outputs);

    /**
     * @brief Set an output value for this context.
     *
     * @param output_name output name
     * @param output output value
     */
    void set_output(const std::string& output_name, mrc::pymrc::JSONValues&& output);

    /**
     * @brief Set the output names to propagate from this context when using pop.
     *
     * @param output_names output names to propagate
     */
    void set_output_names(std::vector<std::string> output_names);

    void outputs_complete();

    /**
     * @brief Get all outputs for this context.
     *
     * @return const mrc::pymrc::JSONValues&
     */
    const mrc::pymrc::JSONValues& view_outputs() const;

    /**
     * @brief Set the row mask indicating which rows of the dataframe are being used to populate the inputs.
     * This should only be called by the first node in an LLM Engine, typically the Extractor node.
     *
     * @param row_mask vector of bools
     */
    void set_row_mask(std::vector<bool>&& row_mask);

    /**
     * @brief Check if the row mask has been set.
     *
     * @return true if row mask has been set
     * @return false if row mask has not been set
     */
    bool has_row_mask() const;

    /**
     * @brief Get the row mask indicating which rows of the dataframe the outputs should be written to.
     * This should only be called by the task handler.
     *
     * @return vector of bools
     */
    const std::vector<bool>& get_row_mask() const;

  private:
    input_mappings_t::const_iterator find_input(const std::string& node_name, bool throw_if_not_found = true) const;

    std::shared_ptr<LLMContext> m_parent{nullptr};
    std::string m_name;
    input_mappings_t m_inputs;
    std::vector<std::string> m_output_names;  // Names of keys to be used as the output. Empty means use all keys

    std::shared_ptr<LLMContextState> m_state;

    mrc::pymrc::JSONValues m_outputs;

    mrc::Promise<void> m_outputs_promise;
    mrc::SharedFuture<void> m_outputs_future;
};
}  // namespace morpheus::llm
