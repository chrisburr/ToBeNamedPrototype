#include "utility/Logger.hxx"
#include <Math/Vector4D.h>

namespace quantities {
auto pt(auto df, std::vector<std::string> &varSet,
        const std::string &outputname, const std::string &inputvector) {
    varSet.push_back(outputname);
    return df.Define(
        outputname,
        [](const ROOT::Math::PtEtaPhiMVector &p4) { return p4.pt(); },
        {inputvector});
}
auto eta(auto df, std::vector<std::string> &varSet,
         const std::string &outputname, const std::string &inputvector) {
    varSet.push_back(outputname);
    return df.Define(
        outputname,
        [](const ROOT::Math::PtEtaPhiMVector &p4) { return p4.eta(); },
        {inputvector});
}
auto phi(auto df, std::vector<std::string> &varSet,
         const std::string &outputname, const std::string &inputvector) {
    varSet.push_back(outputname);
    return df.Define(
        outputname,
        [](const ROOT::Math::PtEtaPhiMVector &p4) { return p4.phi(); },
        {inputvector});
}
auto m_vis(auto df, std::vector<std::string> &varSet,
           const std::string &outputname,
           const std::vector<std::string> &inputvectors) {
    varSet.push_back(outputname);
    // build visible mass from the two particles
    return df.Define(
        outputname,
        [](const ROOT::Math::PtEtaPhiMVector &p4_1,
           const ROOT::Math::PtEtaPhiMVector &p4_2) {
            auto const dileptonsystem = p4_1 + p4_2;
            return dileptonsystem.mass();
        },
        inputvectors);
}
} // end namespace quantities
