#include "ai.h"

AI::AI()
{
    try {
        model = torch::jit::load("c:\\hauken\\18.pt", torch::kCPU);
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope" << e.msg();
    }

    classes << "Jammer" << "Bredbånda pulsgreie" << "Diverse" << "Fraunhofer" << "Tenningsstøy" << "Selvsving";

    // Test data
    /*float test[90][1200];
    for (int i=0; i<90; i++) {
        for (int j=0; j<1200; j++)
            test[i][j] = 0.5 + 1000 * (1.0/rand());
        qDebug() << test[i][90];
    }
    qDebug() << gnssAI(test);*/
}

QString AI::gnssAI(float traceBuffer[90][1200])
{

    qDebug() << "Running AI on GNSS data";

    // Convert incomming data to a tensor
    torch::Tensor tensor;
    try {
        tensor = torch::from_blob(traceBuffer, {1, 1, 90, 1200}, torch::kCPU);
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope tensor" << e.msg();
    }

    // Normalize data
    torch::Tensor min_value;
    torch::Tensor max_value;
    try {
        min_value = tensor.min(); // torch::min(tensor);
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope min" << e.msg();
    }

    try {
        max_value = torch::max(tensor);
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope max" << e.msg();
    }

    try {
        tensor = (tensor - min_value) / (max_value - min_value);
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope tensor2" << e.msg();
    }

    torch::Tensor tensor_augemented;
    try {
        tensor_augemented = torch::repeat_interleave(tensor, 13, 2);
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope aug" << e.msg();
    }

    std::vector<torch::jit::IValue> input;
    try {
        input = {tensor_augemented};
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope input" << e.msg();
    }

    // Make a prediction
    at::Tensor output;
    try {
        output = model.forward(input).toTensor();
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope input" << e.msg();
    }

    int prediction;
    try {
        prediction = output.argmax(1).item().toInt();
    }
    catch(const c10::Error& e) {
        qDebug() << "Nope input" << e.msg();
    }

    return classes[prediction];
}

void AI::receiveBuffer(QVector<QVector<float >> buffer)
{
    if (buffer.size() != 90 || buffer.at(89).size() != 1200) {
        qDebug() << "AI data failed sanity checks, aborting";
    }
    else {
    float tmpBuffer[90][1200];
    for (int i = 0; i < 90; i++) {
        for (int j=0; j < 1200; j++) tmpBuffer[i][j] = (float)buffer.at(i)[j] / 10.0;
    }
    qDebug() << "AI result" << gnssAI(tmpBuffer);
    }
}

