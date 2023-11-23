#include "ai.h"

AI::AI()
{
}

QString AI::gnssAI(float data[90][1200])
{

/*    cout << "Running AI on GNSS data" << endl;

    // Convert incomming data to a tensor
    torch::Tensor tensor = torch::from_blob(data, {1, 1, 90,1200});

    //cout << tensor.sizes() << endl;

    // Normalize data
    torch::Tensor min_value = torch::min(tensor);
    torch::Tensor max_value = torch::max(tensor);
    tensor = (tensor - min_value) / (max_value - min_value);

    // Repeat the tensor to get more square data
    torch::Tensor tensor_augemented = torch::repeat_interleave(tensor, 13, 2);

    vector<torch::jit::IValue> input = {tensor_augemented};

    // Resnet (Currently not the correct model)
    string path_model = "/home/espen/code/hauken_ai/test.pt";
    torch::jit::script::Module model = torch::jit::load(path_model, torch::kCPU);

    // Make a prediction
    at::Tensor output = model.forward(input).toTensor();
    int prediction = output.argmax(1).item().toInt();

    // Does not need to be inialized each time how to put it Global
    const char* classes[6] = {"Jammer",
                              "Bredbånda pulsgreie",
                            "Diverse",
                              "Fraunhofer",
                              "Tenningstøy",
                            "Selvsving"};

    return classes[prediction];
*/
    return QString("Nope");
}
