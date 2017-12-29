#include <opencv2/core/core.hpp>
#include <opencv2/ml/ml.hpp>

#include "json.hpp"
#include "vector_op.h"
#include "svm.h"

const unsigned int DATA_DIMENSIONS = 2; // for Hu moments, it will be 7

using namespace cv;
using json = nlohmann::json;

std::vector<int> svm_prepare_labels_from_json(const json& data_json)
{
    std::vector<int> labels;
    size_t labeled_dataset_size = 0;

    // For each acorn read its learning label
    for (const auto& element : data_json)
    {
        // Ignore not labeled elements
        if (element["label"] == 0)
        {
            continue;
        }
        else
        {
            std::cout << "Label " << static_cast<int>(element["label"]) << std::endl;

            // Read training label
            labels.push_back(element["label"]);
            labeled_dataset_size++;
        }
    }

    assert(labels.size() == labeled_dataset_size);

    // Put training data labels in format for OpenCV SVM
    return labels;
}

cv::Mat svm_prepare_data_from_json(const json& data_json, const std::vector<int>& interesting_moments_indexes = {})
{
    // Set up training data
    std::vector<float> training_data;
    size_t labeled_dataset_size = 0;

    // For each acorn read its Hu moments
    for (const auto& element : data_json)
    {
        // Ignore not labeled elements
        if (element["label"] == 0)
        {
            continue;
        }

        labeled_dataset_size++;

        // Read training data
        std::vector<float> training_element = string_to_vector<float>(element["hu_moments"]);

        if (!interesting_moments_indexes.empty())
        {
            training_element = vector_subset(training_element, interesting_moments_indexes);
            std::cout << "Subset of a vector:" << std::endl;
            print_vector(training_element);
        }

        // Put new element at the end of vector containing all data. The data will be then reshaped.
        if (training_element.size() > 0)
        {
            training_data.insert(
                training_data.end(),
                training_element.begin(),
                training_element.end());
        }
    }

    // Prepare data in format for SVM
    // Create a matrix from vector and reshape matrix to have one training element in one row
    // It is done this way because there's no conversion between vector<vector<float> > and cv::Mat
    assert(DATA_DIMENSIONS*labeled_dataset_size == training_data.size());
    return cv::Mat(training_data).reshape(0, labeled_dataset_size);
}

bool svm_test(const CvSVM& svm, const json& test_data_json, const std::vector<int>& interesting_moments_indexes = {})
{
    const cv::Mat test_data_mat = svm_prepare_data_from_json(test_data_json, interesting_moments_indexes);
    const std::vector<int> labels = svm_prepare_labels_from_json(test_data_json);
    bool is_result_correct = true;
    unsigned int correct_count = 0;
    unsigned int total_count = 0;

    for(auto i = 0; i < test_data_mat.rows; ++i)
    {
        int expected = labels[i];
        if(expected == 0)
        {
            // Do not test data which was not labeled
            continue;
        }
        total_count++;

        int predicted = svm.predict(test_data_mat.row(i));

        if(expected != predicted)
        {
            std::cout << "Incorrect result at row " << i << " with data: "
                      << test_data_mat.row(i) << "\n";
            std::cout << "Predicted: " << predicted << " expected: " << expected << "\n";
            is_result_correct = false;
        }
        else
        {
            std::cout << "Correct! Expected: " << labels[i] << std::endl;
            correct_count++;
        }
    }

    float accuracy = static_cast<float>(correct_count) / static_cast<float>(total_count);
    std::cout << "SVM recognition accuracy: " << 100*accuracy << "%" << std::endl;

    return is_result_correct;
}
