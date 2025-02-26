# Copyright 2024-2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from typing import Any, Dict

from genai_perf.inputs.converters.base_converter import BaseConverter
from genai_perf.inputs.input_constants import DEFAULT_OUTPUT_TOKENS_MEAN
from genai_perf.inputs.inputs_config import InputsConfig
from genai_perf.inputs.retrievers.generic_dataset import GenericDataset
from genai_perf.utils import sample_bounded_normal


class TritonGenerateConverter(BaseConverter):

    def check_config(self, config: InputsConfig) -> None:
        if config.output_tokens_deterministic:
            raise ValueError(
                "The --output-tokens-deterministic flag is not supported for Triton Generate."
            )

    def convert(
        self,
        generic_dataset: GenericDataset,
        config: InputsConfig,
    ) -> Dict[Any, Any]:
        request_body: Dict[str, Any] = {"data": []}

        for file_data in generic_dataset.files_data.values():
            for _, row in enumerate(file_data.rows):
                prompt = row.texts

                payload = {
                    "text_input": prompt,
                }
                request_body["data"].append(
                    self._finalize_payload(
                        payload,
                        config,
                        row,
                    )
                )

        return request_body

    def _add_request_params(
        self, payload: Dict, config: InputsConfig, optional_data: Dict[Any, Any]
    ) -> None:
        if config.add_stream:
            payload["stream"] = True
        max_tokens = self._get_max_tokens(config, optional_data)
        if max_tokens != DEFAULT_OUTPUT_TOKENS_MEAN:
            payload["max_tokens"] = max_tokens
        for key, value in config.extra_inputs.items():
            payload[key] = value
