# SPDX-FileCopyrightText: Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
from unittest import mock

import pytest
from langchain_core.messages import BaseMessage
from langchain_core.messages import ChatMessage
from langchain_core.outputs import ChatGeneration
from langchain_core.outputs import LLMResult

from morpheus.llm.services.llm_service import LLMClient
from morpheus.llm.services.nvfoundation_llm_service import NVFoundationLLMClient
from morpheus.llm.services.nvfoundation_llm_service import NVFoundationLLMService


@pytest.mark.usefixtures("restore_environ")
@pytest.mark.parametrize("api_key", [None, "test_api_key"])
@pytest.mark.parametrize("set_env", [True, False])
def test_constructor(mock_nvfoundationllm: mock.MagicMock, api_key: str, set_env: bool):
    """
    Test that the constructor prefers explicit arguments over environment variables.
    """
    env_api_key = "test_env_api_key"

    if set_env:
        os.environ["NVIDIA_API_KEY"] = env_api_key

    service = NVFoundationLLMService(api_key=api_key)

    expected_api_key = api_key if "NVIDIA_API_KEY" not in os.environ else env_api_key

    assert service.api_key == expected_api_key


def test_get_client():
    service = NVFoundationLLMService(api_key="test_api_key")
    client = service.get_client(model_name="test_model")

    assert isinstance(client, NVFoundationLLMClient)


def test_model_kwargs():
    service = NVFoundationLLMService(arg1="default_value1", arg2="default_value2")

    client = service.get_client(model_name="model_name", arg2="value2")

    assert client.model_kwargs["arg1"] == "default_value1"
    assert client.model_kwargs["arg2"] == "value2"


def test_get_input_names():
    client = NVFoundationLLMService(api_key="nvapi-...").get_client(model_name="test_model", additional_arg="test_arg")

    assert client.get_input_names() == ["prompt"]


def test_generate():
    with mock.patch("langchain_nvidia_ai_endpoints.ChatNVIDIA.generate_prompt", autospec=True) as mock_nvfoundationllm:

        def mock_generation_side_effect(*args, **kwargs):
            return LLMResult(generations=[[
                ChatGeneration(message=ChatMessage(content=x.text, role="assistant")) for x in kwargs["prompts"]
            ]])

        mock_nvfoundationllm.side_effect = mock_generation_side_effect

        client = NVFoundationLLMService(api_key="nvapi-...").get_client(model_name="test_model")
        assert client.generate(prompt="test_prompt") == "test_prompt"


def test_generate_batch():

    with mock.patch("langchain_nvidia_ai_endpoints.ChatNVIDIA.generate_prompt", autospec=True) as mock_nvfoundationllm:

        def mock_generation_side_effect(*args, **kwargs):
            return LLMResult(generations=[[ChatGeneration(message=ChatMessage(content=x.text, role="assistant"))]
                                          for x in kwargs["prompts"]])

        mock_nvfoundationllm.side_effect = mock_generation_side_effect

        client = NVFoundationLLMService(api_key="nvapi-...").get_client(model_name="test_model")

        assert client.generate_batch({'prompt': ["prompt1", "prompt2"]}) == ["prompt1", "prompt2"]


async def test_generate_async():

    with mock.patch("langchain_nvidia_ai_endpoints.ChatNVIDIA.agenerate_prompt", autospec=True) as mock_nvfoundationllm:

        def mock_generation_side_effect(*args, **kwargs):
            return LLMResult(generations=[[ChatGeneration(message=ChatMessage(content=x.text, role="assistant"))]
                                          for x in kwargs["prompts"]])

        mock_nvfoundationllm.side_effect = mock_generation_side_effect

        client = NVFoundationLLMService(api_key="nvapi-...").get_client(model_name="test_model")

        assert await client.generate_async(prompt="test_prompt") == "test_prompt"


async def test_generate_batch_async():

    with mock.patch("langchain_nvidia_ai_endpoints.ChatNVIDIA.agenerate_prompt", autospec=True) as mock_nvfoundationllm:

        def mock_generation_side_effect(*args, **kwargs):
            return LLMResult(generations=[[ChatGeneration(message=ChatMessage(content=x.text, role="assistant"))]
                                          for x in kwargs["prompts"]])

        mock_nvfoundationllm.side_effect = mock_generation_side_effect

        client = NVFoundationLLMService(api_key="nvapi-...").get_client(model_name="test_model")

        assert await client.generate_batch_async({'prompt': ["prompt1", "prompt2"]})


async def test_generate_batch_async_error():
    with mock.patch("langchain_nvidia_ai_endpoints.ChatNVIDIA.agenerate_prompt", autospec=True) as mock_nvfoundationllm:

        def mock_generation_side_effect(*args, **kwargs):
            return LLMResult(generations=[[ChatGeneration(message=ChatMessage(content=x.text, role="assistant"))]
                                          for x in kwargs["prompts"]])

        mock_nvfoundationllm.side_effect = mock_generation_side_effect

        client = NVFoundationLLMService(api_key="nvapi-...").get_client(model_name="test_model")

        with pytest.raises(RuntimeError, match="unittest"):
            await client.generate_batch_async({'prompt': ["prompt1", "prompt2"]})
