/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 *  @file DiscreteConditional.cpp
 *  @date Feb 14, 2011
 *  @author Duy-Nguyen Ta
 *  @author Frank Dellaert
 */

#include <gtsam/discrete/DiscreteConditional.h>
#include <gtsam/discrete/Signature.h>
#include <gtsam/inference/Conditional-inst.h>
#include <gtsam/base/Testable.h>
#include <gtsam/base/debug.h>

#include <boost/make_shared.hpp>

#include <algorithm>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace gtsam {

// Instantiate base class
template class GTSAM_EXPORT Conditional<DecisionTreeFactor, DiscreteConditional> ;

/* ******************************************************************************** */
DiscreteConditional::DiscreteConditional(const size_t nrFrontals,
    const DecisionTreeFactor& f) :
    BaseFactor(f / (*f.sum(nrFrontals))), BaseConditional(nrFrontals) {
}

/* ******************************************************************************** */
DiscreteConditional::DiscreteConditional(const DecisionTreeFactor& joint,
    const DecisionTreeFactor& marginal) :
    BaseFactor(
        ISDEBUG("DiscreteConditional::COUNT") ? joint : joint / marginal), BaseConditional(
            joint.size()-marginal.size()) {
  if (ISDEBUG("DiscreteConditional::DiscreteConditional"))
    cout << (firstFrontalKey()) << endl; //TODO Print all keys
}

/* ******************************************************************************** */
DiscreteConditional::DiscreteConditional(const DecisionTreeFactor& joint,
    const DecisionTreeFactor& marginal, const Ordering& orderedKeys) :
    DiscreteConditional(joint, marginal) {
  keys_.clear();
  keys_.insert(keys_.end(), orderedKeys.begin(), orderedKeys.end());
}

/* ******************************************************************************** */
DiscreteConditional::DiscreteConditional(const Signature& signature)
    : BaseFactor(signature.discreteKeys(), signature.cpt()),
      BaseConditional(1) {}

/* ******************************************************************************** */
void DiscreteConditional::print(const string& s,
                                const KeyFormatter& formatter) const {
  cout << s << " P( ";
  for (const_iterator it = beginFrontals(); it != endFrontals(); ++it) {
    cout << formatter(*it) << " ";
  }
  if (nrParents()) {
    cout << "| ";
    for (const_iterator it = beginParents(); it != endParents(); ++it) {
      cout << formatter(*it) << " ";
    }
  }
  cout << ")";
  Potentials::print("");
  cout << endl;
}

/* ******************************************************************************** */
bool DiscreteConditional::equals(const DiscreteFactor& other,
    double tol) const {
  if (!dynamic_cast<const DecisionTreeFactor*>(&other))
    return false;
  else {
    const DecisionTreeFactor& f(
        static_cast<const DecisionTreeFactor&>(other));
    return DecisionTreeFactor::equals(f, tol);
  }
}

/* ******************************************************************************** */
Potentials::ADT DiscreteConditional::choose(
    const DiscreteValues& parentsValues) const {
  // Get the big decision tree with all the levels, and then go down the
  // branches based on the value of the parent variables.
  ADT pFS(*this);
  size_t value;
  for (Key j : parents()) {
    try {
      value = parentsValues.at(j);
      pFS = pFS.choose(j, value);  // ADT keeps getting smaller.
    } catch (exception&) {
      cout << "Key: " << j << "  Value: " << value << endl;
      parentsValues.print("parentsValues: ");
      throw runtime_error("DiscreteConditional::choose: parent value missing");
    };
  }
  return pFS;
}

/* ******************************************************************************** */
DecisionTreeFactor::shared_ptr DiscreteConditional::chooseAsFactor(
    const DiscreteValues& parentsValues) const {
  ADT pFS = choose(parentsValues);

  // Convert ADT to factor.
  if (nrFrontals() != 1) {
    throw std::runtime_error("Expected only one frontal variable in choose.");
  }
  DiscreteKeys keys;
  const Key frontalKey = keys_[0];
  size_t frontalCardinality = this->cardinality(frontalKey);
  keys.push_back(DiscreteKey(frontalKey, frontalCardinality));
  return boost::make_shared<DecisionTreeFactor>(keys, pFS);
}

/* ******************************************************************************** */
void DiscreteConditional::solveInPlace(DiscreteValues* values) const {
  // TODO: Abhijit asks: is this really the fastest way? He thinks it is.
  ADT pFS = choose(*values); // P(F|S=parentsValues)

  // Initialize
  DiscreteValues mpe;
  double maxP = 0;

  DiscreteKeys keys;
  for(Key idx: frontals()) {
    DiscreteKey dk(idx, cardinality(idx));
    keys & dk;
  }
  // Get all Possible Configurations
  vector<DiscreteValues> allPosbValues = cartesianProduct(keys);

  // Find the MPE
  for(DiscreteValues& frontalVals: allPosbValues) {
    double pValueS = pFS(frontalVals); // P(F=value|S=parentsValues)
    // Update MPE solution if better
    if (pValueS > maxP) {
      maxP = pValueS;
      mpe = frontalVals;
    }
  }

  //set values (inPlace) to mpe
  for(Key j: frontals()) {
    (*values)[j] = mpe[j];
  }
}

/* ******************************************************************************** */
void DiscreteConditional::sampleInPlace(DiscreteValues* values) const {
  assert(nrFrontals() == 1);
  Key j = (firstFrontalKey());
  size_t sampled = sample(*values); // Sample variable given parents
  (*values)[j] = sampled; // store result in partial solution
}

/* ******************************************************************************** */
size_t DiscreteConditional::solve(const DiscreteValues& parentsValues) const {

  // TODO: is this really the fastest way? I think it is.
  ADT pFS = choose(parentsValues); // P(F|S=parentsValues)

  // Then, find the max over all remaining
  // TODO, only works for one key now, seems horribly slow this way
  size_t mpe = 0;
  DiscreteValues frontals;
  double maxP = 0;
  assert(nrFrontals() == 1);
  Key j = (firstFrontalKey());
  for (size_t value = 0; value < cardinality(j); value++) {
    frontals[j] = value;
    double pValueS = pFS(frontals); // P(F=value|S=parentsValues)
    // Update MPE solution if better
    if (pValueS > maxP) {
      maxP = pValueS;
      mpe = value;
    }
  }
  return mpe;
}

/* ******************************************************************************** */
size_t DiscreteConditional::sample(const DiscreteValues& parentsValues) const {
  static mt19937 rng(2);  // random number generator

  // Get the correct conditional density
  ADT pFS = choose(parentsValues);  // P(F|S=parentsValues)

  // TODO(Duy): only works for one key now, seems horribly slow this way
  assert(nrFrontals() == 1);
  Key key = firstFrontalKey();
  size_t nj = cardinality(key);
  vector<double> p(nj);
  DiscreteValues frontals;
  for (size_t value = 0; value < nj; value++) {
    frontals[key] = value;
    p[value] = pFS(frontals);  // P(F=value|S=parentsValues)
    if (p[value] == 1.0) {
      return value;  // shortcut exit
    }
  }
  std::discrete_distribution<size_t> distribution(p.begin(), p.end());
  return distribution(rng);
}

/* ******************************************************************************** */

}// namespace
