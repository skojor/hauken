#include "ai.h"

AI::AI()
{
/*  try {
        torch::jit::script::Module model = torch::jit::load("c:\\hauken\\18.pt", torch::kCPU);
    }
    catch(...) {
        qDebug() << "Nope";
    }*/

    classes << "Jammer" << "Bredbånda pulsgreie" << "Diverse" << "Fraunhofer" << "Tenningsstøy" << "Selvsving";
}

QString AI::gnssAI(float traceBuffer[90][1200])
{

    /*qDebug() << "Running AI on GNSS data";

    // Convert incomming data to a tensor
    at::IntArrayRef sizes = {1, 1, 90, 1200};
    torch::Tensor tensor = torch::from_blob(traceBuffer, sizes); //{1, 1, 90, 1200});

    // Normalize data
    torch::Tensor min_value = torch::min(tensor);
    torch::Tensor max_value = torch::max(tensor);
    tensor = (tensor - min_value) / (max_value - min_value);

    // Repeat the tensor to get more square data
    torch::Tensor tensor_augemented = torch::repeat_interleave(tensor, 13, 2);

    std::vector<torch::jit::IValue> input = {tensor_augemented};

    // Resnet (Currently not the correct model)
    torch::string path_model = "c:\\hauken\\18.pt";
    torch::jit::script::Module model = torch::jit::load(path_model, torch::kCPU);

    // Make a prediction
    at::Tensor output = model.forward(input).toTensor();
    int prediction = output.argmax(1).item().toInt();

    return classes[prediction];*/
    return "none";
}

void AI::receiveBuffer(QVector<QVector<float >> buffer)
{
    float tmpBuffer[90][1200];
    for (int i = 0; i < 90; i++) {
        for (int j=0; j < 1200; j++) tmpBuffer[i][j] = (float)buffer.at(i)[j] / 10.0;
    }
    qDebug() << "AI result" << gnssAI(tmpBuffer);
}
