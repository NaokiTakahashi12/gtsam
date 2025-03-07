/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    DiscreteBayesTree.cpp
 * @brief   Discrete Bayes Tree, the result of eliminating a DiscreteJunctionTree
 * @brief   DiscreteBayesTree
 * @author  Frank Dellaert
 * @author  Richard Roberts
 */

#include <gtsam/base/treeTraversal-inst.h>
#include <gtsam/inference/BayesTree-inst.h>
#include <gtsam/inference/BayesTreeCliqueBase-inst.h>
#include <gtsam/discrete/DiscreteBayesTree.h>
#include <gtsam/discrete/DiscreteBayesNet.h>

namespace gtsam {

  // Instantiate base class
  template class BayesTreeCliqueBase<DiscreteBayesTreeClique, DiscreteFactorGraph>;
  template class BayesTree<DiscreteBayesTreeClique>;

  /* ************************************************************************* */
  double DiscreteBayesTreeClique::evaluate(
      const DiscreteValues& values) const {
    // evaluate all conditionals and multiply
    double result = (*conditional_)(values);
    for (const auto& child : children) {
      result *= child->evaluate(values);
    }
    return result;
  }

  /* ************************************************************************* */
  bool DiscreteBayesTree::equals(const This& other, double tol) const {
    return Base::equals(other, tol);
  }

  /* ************************************************************************* */
  double DiscreteBayesTree::evaluate(
      const DiscreteValues& values) const {
    double result = 1.0;
    for (const auto& root : roots_) {
      result *= root->evaluate(values);
    }
    return result;
  }

} // \namespace gtsam




