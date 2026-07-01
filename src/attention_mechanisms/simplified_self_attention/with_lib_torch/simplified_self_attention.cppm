//
// Created by Adeel on 6/26/26.
//

module;

#include<torch/torch.h>

export module simplified_self_attention;

namespace SimplifiedSelfAttention {
    export template<int64_t Rows, int64_t Cols>
    auto basic_attention_score(const torch::Tensor &inputs) {
        TORCH_CHECK(inputs.size(0) == Rows && inputs.size(1) == Cols, "Expected shape {", Rows, ", ", Cols,
                    "}, but got ", inputs.sizes())

        auto const query = inputs[1];
        auto attn_scores_2 = torch::empty(inputs.size(0));

        for (int64_t i = 0; i < inputs.size(0); ++i) {
            // mat[i] creates a sub-tensor slice representing row i
            attn_scores_2[i] = torch::dot(inputs[i], query);
        }

        return attn_scores_2;
    }

    export auto normalize(const torch::Tensor &attention_scores) {
        auto att_tmp = attention_scores / attention_scores.sum();
        return att_tmp;
    }

    export auto softmax_naive(const torch::Tensor &attention_score) {
        return torch::exp(attention_score) / torch::exp(attention_score).sum(0);
    }

    export template<int64_t Rows, int64_t Cols>
    auto compute_context_vector(const torch::Tensor &inputs) {
        TORCH_CHECK(inputs.size(0) == Rows && inputs.size(1) == Cols, "Expected shape {", Rows, ", ", Cols,
                    "}, but got ", inputs.sizes())
        auto query = inputs[1];
        auto attention_score = basic_attention_score<Rows, Cols>(inputs);
        auto attention_weights = torch::softmax(attention_score, 0);


        auto context_vec = torch::zeros(query.sizes());
        for (int64_t i = 0; i < inputs.size(0); i++) {
            context_vec = context_vec + (attention_weights[i] * inputs[i]);
        }
        return context_vec;
    }
}
