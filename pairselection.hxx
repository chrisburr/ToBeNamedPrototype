#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "TVector2.h"
#include "utility/Logger.hxx"
#include "utility/utility.hxx"

// TODO general: enable proper logging at COMPILE (!!!) time.
//               use namespaces appropriately in functions, and use "using" to
//               make types shorter.
//
// Examples: void foo() {
//    using namespace ROOT::VecOps;
//    }
//
//    using VecF = const ROOT::RVec<float>&;

namespace pairselection {

auto filterGoodPairs(auto df, const std::string &pairname,
                     const std::string &filtername) {
    using namespace ROOT::VecOps;
    return df.Filter(
        [](const ROOT::RVec<int> &pair) {
            const auto goodParticles =
                Filter(pair, [](int i) { return -1 != i; });
            return Any(goodParticles);
        },
        {pairname}, filtername);
}

auto compareForPairs(const ROOT::RVec<float> &lep1pt,
                     const ROOT::RVec<float> &lep1iso,
                     const ROOT::RVec<float> &lep2pt,
                     const ROOT::RVec<float> &lep2iso) {
    return [lep1pt, lep1iso, lep2pt, lep2iso](auto value_next,
                                              auto value_previous) {
        Logger::get("PairSelectionCompare")->debug("lep1 Pt: {}", lep1pt);
        Logger::get("PairSelectionCompare")->debug("lep1 Iso: {}", lep1iso);
        Logger::get("PairSelectionCompare")->debug("lep2 Pt: {}", lep2pt);
        Logger::get("PairSelectionCompare")->debug("lep2 Iso: {}", lep2iso);

        Logger::get("PairSelectionCompare")
            ->debug("Next pair: {}, {}", std::to_string(value_next.first),
                    std::to_string(value_next.second));
        Logger::get("PairSelectionCompare")
            ->debug("Previous pair: {}, {}",
                    std::to_string(value_previous.first),
                    std::to_string(value_previous.second));
        const auto i1_next = value_next.first;
        const auto i1_previous = value_previous.second;

        // start with lep1 isolation
        const auto iso1_next = lep1iso[i1_next];
        const auto iso1_previous = lep1iso[i1_previous];
        Logger::get("PairSelectionCompare")
            ->debug("Isolations: {}, {}", iso1_next, iso1_previous);
        if (not utility::ApproxEqual(iso1_next, iso1_previous)) {
            return iso1_next > iso1_previous;
        } else {
            // if too similar, compare lep1 pt
            Logger::get("PairSelectionCompare")
                ->debug("Isolation lep 1 too similar, taking pt");
            const auto pt1_next = lep1pt[i1_next];
            const auto pt1_previous = lep1pt[i1_previous];
            if (not utility::ApproxEqual(pt1_next, pt1_previous)) {
                return pt1_next > pt1_previous;
            } else {
                // if too similar, compare lep2 iso
                const auto i2_next = value_next.first;
                const auto i2_previous = value_previous.second;
                Logger::get("PairSelectionCompare")
                    ->debug("Pt lep 1 too similar, taking lep2 iso");
                const auto iso2_next = lep2iso[i2_next];
                const auto iso2_previous = lep2iso[i2_previous];
                if (not utility::ApproxEqual(iso2_next, iso2_previous)) {
                    return iso2_next > iso2_previous;
                } else {
                    // if too similar, compare lep2 pt
                    Logger::get("PairSelectionCompare")
                        ->debug("Isolation lep 2 too similar, taking pt");
                    const auto pt2_next = lep2pt[i2_next];
                    const auto pt2_previous = lep2pt[i2_previous];
                    return pt2_next > pt2_previous;
                }
            }
        }
    };
}

namespace mutau {
// 1. Muon Isolation
// 2. Muon pt
// 3. Tau Isolation
// 4. Tau pt

auto PairSelectionAlgo() {
    Logger::get("PairSelection")->debug("Setting up algorithm");
    return [](const ROOT::RVec<float> &taupt, const ROOT::RVec<float> &tauiso,
              const ROOT::RVec<float> &muonpt, const ROOT::RVec<float> &muoniso,
              const ROOT::RVec<int> &taumask, const ROOT::RVec<int> &muonmask) {
        ROOT::RVec<int> selected_pair; // first entry is the muon index, second
                                       // entry is the tau index
        const auto original_tau_indices = ROOT::VecOps::Nonzero(taumask);
        const auto original_muon_indices = ROOT::VecOps::Nonzero(muonmask);

        if (original_tau_indices.size() == 0 or
            original_muon_indices.size() == 0) {
            selected_pair = {-1, -1};
            return selected_pair;
        }
        Logger::get("PairSelection")
            ->debug("Running algorithm on good taus and muons");

        const auto selected_taupt =
            ROOT::VecOps::Take(taupt, original_tau_indices);
        const auto selected_tauiso =
            ROOT::VecOps::Take(tauiso, original_tau_indices);
        const auto selected_muonpt =
            ROOT::VecOps::Take(muonpt, original_muon_indices);
        const auto selected_muoniso =
            ROOT::VecOps::Take(muoniso, original_muon_indices);

        const auto pair_indices = ROOT::VecOps::Combinations(
            selected_muonpt, selected_taupt); // Gives indices of mu-tau pair
        Logger::get("PairSelection")->debug("Pairs: ", pair_indices);

        // TODO, try out std::pair<UInt_t>, or std::tuple<UInt_t>.
        const auto pairs = ROOT::VecOps::Construct<std::pair<UInt_t, UInt_t>>(
            pair_indices[0], pair_indices[1]);
        Logger::get("PairSelection")->debug("Pairs size: {}", pairs.size());
        Logger::get("PairSelection")
            ->debug("Constituents pair 0: {} {}", pairs[0].first,
                    pairs[0].second);

        if (pairs.size() > 1) {
            Logger::get("PairSelection")
                ->debug("Constituents pair 1: {} {}",
                        std::to_string(pairs[1].first),
                        std::to_string(pairs[1].second));
        }

        const auto sorted_pairs = ROOT::VecOps::Sort(
            pairs, compareForPairs(selected_muonpt, -1. * selected_muoniso,
                                   selected_taupt, selected_tauiso));

        Logger::get("PairSelection")->debug("TauPt: {}", selected_taupt);
        Logger::get("PairSelection")->debug("TauIso: {}", selected_tauiso);
        Logger::get("PairSelection")->debug("MuonPt: {}", selected_muonpt);
        Logger::get("PairSelection")->debug("MuonIso: {}", selected_muoniso);

        const auto selected_mu_index = sorted_pairs[0].first;
        const auto selected_tau_index = sorted_pairs[0].second;
        selected_pair = {
            static_cast<int>(original_muon_indices[selected_mu_index]),
            static_cast<int>(original_tau_indices[selected_tau_index])};
        Logger::get("PairSelection")
            ->debug("Selected original pair indices: mu = {} , tau = {}",
                    selected_pair[0], selected_pair[1]);
        Logger::get("PairSelection")
            ->debug("mu(Pt) = {} , tau(Pt) = {} ",
                    muonpt[static_cast<UInt_t>(selected_pair[0])],
                    taupt[static_cast<UInt_t>(selected_pair[1])]);
        Logger::get("PairSelection")
            ->debug("mu(Iso) = {} , tau(Pt) = {} ",
                    muoniso[static_cast<UInt_t>(selected_pair[0])],
                    tauiso[static_cast<UInt_t>(selected_pair[1])]);

        return selected_pair;
    };
}

auto PairSelection(auto df, const std::string taumask,
                   const std::string muonmask, const std::string pairname,
                   const std::vector<std::string> pairvariables) {
    Logger::get("PairSelection")->debug("Setting up mutau pair building");
    auto df1 = df.Define(pairname, pairselection::mutau::PairSelectionAlgo(),
                         {"Tau_pt", "Tau_rawDeepTau2017v2p1VSjet", "Muon_pt",
                          "Muon_pfRelIso04_all", taumask, muonmask});
    return df1;
}

} // end namespace mutau

} // end namespace pairselection
