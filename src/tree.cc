//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: tree.cc
// Copyright (C) 2012 Toru Shiozaki
//
// Author: Toru Shiozaki <shiozaki@northwestern.edu>
// Maintainer: Shiozaki group
//
// This file is part of the SMITH3 package.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


#include "residual.h"
#include "constants.h"

using namespace std;
using namespace smith;


Tree::Tree(shared_ptr<Equation> eq, string lab) : parent_(NULL), tree_name_(eq->name()), num_(-1), label_(lab), root_targets_(eq->targets()) {
  // First make ListTensor for all the diagrams
  list<shared_ptr<Diagram>> d = eq->diagram();

  const bool rt_targets = eq->targets();

  for (auto& i : d) {
    shared_ptr<ListTensor> tmp = make_shared<ListTensor>(i);
    // All internal tensor should be included in the active part
    tmp->absorb_all_internal();

    // rearrange brakets and reindex associated tensors, ok if not complex
    tmp->absorb_ket();

    shared_ptr<Tensor> first = tmp->front();
    shared_ptr<ListTensor> rest = tmp->rest();

    // reorder to minimize the cost
    rest->reorder();

    // convert to tree and then bc
    shared_ptr<Tree> tr;
    if (label_ == "residual" || label_ == "source" || label_ == "density" || label_ == "density1"
                             || label_ == "density2" || label_.find("deci") != string::npos || label_ == "norm") {
      tr = make_shared<Residual>(rest, lab, rt_targets);
    } else if (label_ == "energy" || label_ == "corr") {
      throw logic_error("removed");
    } else {
      throw logic_error("Error Tree::Tree, code generation for this tree type not implemented");
    }
    list<shared_ptr<Tree>> lt; lt.push_back(tr);
    shared_ptr<BinaryContraction> b = make_shared<BinaryContraction>(lt, first, i->target_index());
    bc_.push_back(b);
  }

  factorize();
  move_up_operator();
  set_parent_sub();
  set_target_rec();
}


Tree::Tree(const shared_ptr<ListTensor> l, string lab, const bool t) : num_(-1), label_(lab), root_targets_(t) {
  target_ = l->target();
  dagger_ = l->dagger();
  if (l->length() > 1) {
    shared_ptr<BinaryContraction> bc = make_shared<BinaryContraction>(target_, l, lab, root_targets_);
    bc_.push_back(bc);
  } else {
    shared_ptr<Tensor> t = l->front();
    t->set_factor(l->fac());
    t->set_scalar(l->scalar());
    op_.push_back(t);
  }
}


BinaryContraction::BinaryContraction(shared_ptr<Tensor> o, shared_ptr<ListTensor> l, string lab, const bool rt) : label_(lab) {
  // o is a target tensor NOT included in l
  target_ = o;
  tensor_ = l->front();
  shared_ptr<ListTensor> rest = l->rest();

  shared_ptr<Tree> tr;
  if (label_ == "residual" || label_ == "source" || label_ == "density" || label_ == "density1"
                           || label_ == "density2" || label_.find("deci") != string::npos || label_ == "norm") {
    tr = make_shared<Residual>(rest, lab, rt);
  } else if (label_ == "energy" || label_ == "corr") {
    throw logic_error("removed");
  } else {
    throw logic_error("Error BinaryContraction::BinaryContraction, code generation for this tree type not implemented");
  }
  subtree_.push_back(tr);

}


void Tree::factorize() {
  list<list<shared_ptr<BinaryContraction>>::iterator> done;
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
    if (find(done.begin(), done.end(), i) != done.end()) continue;
    auto j = i; ++j;
    for ( ; j != bc_.end(); ++j) {
      // bc is factorized according to excitation target indices
      if (*(*i)->tensor() == *(*j)->tensor() && (*i)->dagger() == (*j)->dagger() && (*i)->target_index_str() == (*j)->target_index_str()) {
        done.push_back(j);
        (*i)->subtree().insert((*i)->subtree().end(), (*j)->subtree().begin(), (*j)->subtree().end());
      }
    }
  }
  for (auto& i : done) bc_.erase(i);
  for (auto& i : bc_) i->factorize();
}


void BinaryContraction::factorize() {
  list<list<shared_ptr<Tree>>::iterator> done;
  // and then merge subtrees
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) {
    if (find(done.begin(), done.end(), i) != done.end()) continue;
    auto j = i; ++j;
    for ( ; j != subtree_.end(); ++j) {
      if (find(done.begin(), done.end(), j) != done.end()) continue;
      if ((*i)->merge(*j)) done.push_back(j);
    }
  }
  for (auto i = done.begin(); i != done.end(); ++i) subtree_.erase(*i);
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) (*i)->factorize();
}


void Tree::move_up_operator() {
  for (auto& b : bc_)
    b->move_up_operator();
}


void BinaryContraction::move_up_operator() {
  if (subtree_.size() == 1 && subtree_.front()->can_move_up()) {
    source_ = subtree_.front()->op().front();
    subtree_.clear();
  }

  for (auto& i : subtree_)
    i->move_up_operator();
}


bool Tree::merge(shared_ptr<Tree> o) {
  bool out = false;
  if (o->bc_.size() > 0) {
    if (bc_.size() == 0 || *bc_.front()->tensor() == *o->bc_.front()->tensor()) {
      out = true;
      bc_.insert(bc_.end(), o->bc_.begin(), o->bc_.end());
      for (auto& i : bc_) i->set_target(target_);
    }
  } else if (o->op_.size() > 0) {
    out = true;
    op_.insert(op_.end(), o->op_.begin(), o->op_.end());
  }
  return out;
}


void BinaryContraction::set_target_rec() {
  if (!subtree_.empty()) {
    auto i = subtree_.begin();
    shared_ptr<Tensor> first = (*i)->target();
    for (++i ; i != subtree_.end(); ++i) {
      (*i)->set_target(first);
    }
    for (i = subtree_.begin() ; i != subtree_.end(); ++i)
      (*i)->set_target_rec();
  }
}


void Tree::set_target_rec() {
  if (!bc_.empty()) {
    for (auto& i : bc_) i->set_target_rec();
  }
}


void BinaryContraction::set_parent_subtree() {
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) {
    (*i)->set_parent(this);
    (*i)->set_parent_sub();
  }
}


void Tree::set_parent_sub() {
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
    (*i)->set_parent(this);
    (*i)->set_parent_subtree();
  }
}


int BinaryContraction::depth() const { return parent_->depth(); }

int Tree::depth() const { return parent_ ? parent_->parent()->depth()+1 : 0; }


bool BinaryContraction::dagger() const {
  return subtree_.front()->dagger();
}


shared_ptr<Tensor> BinaryContraction::next_target() {
  return !subtree_.empty() ? subtree().front()->target() : source_;
}


void Tree::sort_gamma(list<shared_ptr<Tensor>> o) {
  gamma_ = o;
  list<shared_ptr<Tensor>> g = gather_gamma();
  for (auto& i : g) find_gamma(i);
}


void Tree::find_gamma(shared_ptr<Tensor> o) {
  bool found = false;
  for (auto& i : gamma_) {
    if ((*i) == (*o)) {
      found = true;
      o->set_alias(i);
      break;
    }
  }
  if (!found) gamma_.push_back(o);
}


// return gamma below this
list<shared_ptr<Tensor>> Tree::gather_gamma() const {
  list<shared_ptr<Tensor>> out;
  for (auto& i : bc_) {
    for (auto& j : i->subtree()) {
      // recursive call
      list<shared_ptr<Tensor>> tmp = j->gather_gamma();
      out.insert(out.end(), tmp.begin(), tmp.end());
    }
    if (i->source() && i->source()->label().find("Gamma") != string::npos)
      out.push_back(i->source());
  }
  for (auto& i : op_)
    if (i->label().find("Gamma") != string::npos) out.push_back(i);
  for (auto& b : bc_)
    if (b->tensor()->label().find("Gamma") != string::npos) out.push_back(b->tensor());
  return out;
}


string BinaryContraction::target_index_str() const {
  stringstream zz;
  if (!target_index_.empty()) {
    zz << "(";
    for (auto i = target_index_.begin(); i != target_index_.end(); ++i) {
      if (i != target_index_.begin()) zz << ", ";
      zz << (*i)->str(false);
    }
    zz << ")";
  }
  return zz.str();
}


void BinaryContraction::print() const {
  string indent = "";
  // I think \"+=\" is more obvious
  for (int i = 0; i != depth(); ++i) indent += "  ";
  cout << indent << (target_ ? (target_->str() + " = ") : "") << tensor_->str() << (depth() == 0 ? target_index_str() : "") << " * "
                 << (subtree_.empty() ? source_->str() : subtree_.front()->target()->str()) << endl;
  // in binary contraction, it has subtrees
//  int count = 0;
  for (auto& i : subtree_) {
//    cout << " subtree number " << count <<" in depth " << depth() <<  "  ";
    i->print();
//    count++;
  }
}


void Tree::print() const {
  string indent = "";
  for (int i = 0; i != depth(); ++i) indent += "  ";
  // print tree contraction
//  int countop = 0;
  if (target_) {
    for (auto i = op_.begin(); i != op_.end(); ++i) {
//      cout << "operator tensor number " << countop << " in depth " << depth() << "  " ;
      cout << indent << target_->str() << " += " << (*i)->str() << (dagger_ ? " *" : "") << endl;
//      countop++;
    }
  }
  // and all possible binary contractions
//  int count = 0;
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
//    cout << "binary contraction number " << count << " in depth " << depth() << "  " ;
//    ++count;
    (*i)->print();
  }
}


// local functions... (not a good practice...) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
static string label__(const string& lbl) {
  string label = lbl;
  if (label == "proj") label = "r";
  size_t found = label.find("dagger");
  if (found != string::npos) {
    string tmp(label.begin(), label.begin() + found);
    label = tmp;
  }
  return label;
}
// local functions... (not a good practice...) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



list<shared_ptr<const Index>> BinaryContraction::target_indices() {
  // returns a list of target indices
  return target_->index();
}


list<shared_ptr<const Index>> BinaryContraction::loop_indices() {
  // returns a list of inner loop indices.
  list<shared_ptr<const Index>> out;
  list<shared_ptr<const Index>> ti = target_->index();
  for (auto iter = tensor_->index().begin(); iter != tensor_->index().end(); ++iter) {
    bool found = false;
    for (auto i = ti.begin(); i != ti.end(); ++i) {
      if ((*i)->identical(*iter)) {
        found = true;
        break;
      }
    }
    if (!found) {
      out.push_back(*iter);
    }
  }
  return out;
}


OutStream Tree::generate_compute_operators(shared_ptr<Tensor> target, const vector<shared_ptr<Tensor>> op, const bool dagger) const {
  OutStream out;

  vector<string> close;
  string cindent = "  ";
  // note that I am using reverse_iterator
  //out.dd << target->generate_loop(cindent, close);
  // get data
  out.dd << target->generate_get_block(cindent, "o", "out()", true);

  // needed in case the tensor labels are repeated..
  vector<shared_ptr<Tensor>> uniq_tensors;
  vector<string> tensor_labels;
  // map redundant tensor label to unique tensor label
  map<string, int> op_tensor_lab;

  int uniq_cnt = 0;
  for (auto& i : op) {
    string label = label__(i->label());
    // if we already have the label in the list
    if (find(tensor_labels.begin(), tensor_labels.end(), label) != tensor_labels.end()) {
      continue;
    }
    // otherwise not yet found, so add to list
    tensor_labels.push_back(label);
    uniq_tensors.push_back(i);
    op_tensor_lab[label] = uniq_cnt;
    ++uniq_cnt;
  }

  // add the source data to the target
  int j = 0;
  for (auto s = op.begin(); s != op.end(); ++s, ++j) {
    stringstream uu; uu << "i" << j;
    out.dd << cindent << "{" << endl;

    // uses map to give label number consistent with operator, needed in case label is repeated (eg ccaa)
    string label = label__((*s)->label());
    stringstream instr; instr << "in(" << op_tensor_lab[label] << ")";

    out.dd << (*s)->generate_get_block(cindent+"  ", uu.str(), instr.str());
    list<shared_ptr<const Index>> di = target->index();
    out.dd << (*s)->generate_sort_indices(cindent+"  ", uu.str(), instr.str(), di, true);
    out.dd << cindent << "}" << endl;

    // if this is at the top-level and needs to be daggered:
    if (depth() == 0 && dagger) {
      shared_ptr<Tensor> top = make_shared<Tensor>(**s);
      // swap operators so that tensor is daggered
      if (top->index().size() != 4) {
         throw logic_error("Daggered object is only supported for 4-index tensors");
      } else {
        auto k0 = di.begin(); auto k1 = k0; ++k1; auto k2 = k1; ++k2; auto k3 = k2; ++k3;
        list<pair<list<shared_ptr<const Index>>::iterator, list<shared_ptr<const Index>>::iterator>> map;
        map.push_back(make_pair(k0, k2));
        map.push_back(make_pair(k2, k0));
        map.push_back(make_pair(k1, k3));
        map.push_back(make_pair(k3, k1));
        list<shared_ptr<const Index>> tmp;
        for (auto& k : top->index()) {
          for (auto l = map.begin(); l != map.end(); ++l) {
            if (k->identical(*l->first)) {
              tmp.push_back(*l->second);
              break;
            }
            auto ll = l;
            if (++ll == map.end()) throw logic_error("should not happen: dagger stuffs");
          }
        }
        top->index() = tmp;
        out.dd << cindent << "{" << endl;
        out.dd << top->generate_get_block(cindent+"  ", uu.str(), instr.str());
        list<shared_ptr<const Index>> di = target->index();
        out.dd << top->generate_sort_indices(cindent+"  ", uu.str(), instr.str(), di, true);
        out.dd << cindent << "}" << endl;
      }
    }
  }

  // put data
  {
    string label = target->label();
    list<shared_ptr<const Index>> ti = target->index();
    out.dd << cindent << "out()->add_block(odata";
    for (auto i = ti.rbegin(); i != ti.rend(); ++i)
      out.dd << ", " << (*i)->str_gen();
    out.dd << ");" << endl;
  }
  for (auto iter = close.rbegin(); iter != close.rend(); ++iter)
    out.dd << *iter << endl;
  return out;
}


OutStream Tree::generate_task_ci(const int ic, const vector<shared_ptr<Tensor>> op, const list<shared_ptr<Tensor>> g, const int iz, const bool diagonal) const {
  OutStream out;

  vector<string> ops;
  for (int j = 0; j != 5; ++j)
    ops.push_back("den" + to_string(j) + "ci");
  for (int j = 0; j != 5; ++j)
    ops.push_back(op[1]->label() + "_" + to_string(j));
  int ip = -1;
  if (parent_) ip = parent_->parent()->num();

  string scalar;
  out << generate_task(ip, ic, ops, scalar, iz, /*der*/false, /*diagonal*/diagonal);

  return out;
}

OutStream Tree::generate_task_gamma(const int ic, const vector<shared_ptr<Tensor>> op, const list<shared_ptr<Tensor>> g, const int iz, const bool diagonal, const bool gamma, const bool merged) const {
  OutStream out;

  vector<string> ops;
  if (gamma)
    for (int j = 0; j != 5; ++j)
      ops.push_back("den" + to_string(j) + "ci");
  else
    ops.push_back("den0ci");
  // Gamma is absorbed in the task, therefore we do not have op[1] here
  ops.push_back(op[2]->label());
  if (merged)
    ops.push_back("f1_");
  int ip = -1;
  if (parent_) ip = parent_->parent()->num();

  string scalar;
  out << generate_task_gamma(ip, ic, ops, scalar, iz, /*der*/false, /*diagonal*/diagonal);

  return out;
}

OutStream Tree::generate_task(const int ic, const vector<shared_ptr<Tensor>> op, const list<shared_ptr<Tensor>> g, const int iz, const bool diagonal) const {
  OutStream out;

  vector<string> ops;
  for (auto& i : op) ops.push_back(i->label());
  int ip = -1;
  if (parent_) ip = parent_->parent()->num();

  string scalar;
  for (auto& i : op) {
    if (!i->scalar().empty()) {
      if (i->scalar() != "e0") throw logic_error("unknown scalar. Tree::generate_task");
      if (!scalar.empty() && i->scalar() != scalar) throw logic_error("multiple scalars. Tree::generate_task");
      scalar = i->scalar();
    }
  }

  // if gamma, we need to add dependency.
  // this one is virtual, ie tree specific
  out << generate_task(ip, ic, ops, scalar, iz, /*der*/false, /*diagonal*/diagonal);

// TODO at this moment all gammas are recomputed.
#if 0
  for (auto& i : op) {
    if (i->label().find("Gamma") != string::npos)
      out.ee << "  task" << ic << "->" << add_depend(i, g) << endl;
  }
  out.ee << endl;
#endif

  return out;
}


string Tree::add_depend(const shared_ptr<const Tensor> o, const list<shared_ptr<Tensor>> gamma) const {
  stringstream out;
  if (!parent_) {
    assert(!gamma.empty());
    for (auto& i : gamma) {
      if (o->label() == i->label()) out << "add_dep(task" << i->num() << ");";
    }
  } else {
    out << parent_->parent()->add_depend(o, gamma);
  }
  return out.str();
}


tuple<OutStream, int, int, vector<shared_ptr<Tensor>>>
  BinaryContraction::generate_task_list(int tcnt, int t0, const list<shared_ptr<Tensor>> gamma, vector<shared_ptr<Tensor>> itensors) const {
  OutStream out, tmp;
  string depends, tasks, specials;

  for (auto& i : subtree_) {
    tie(tmp, tcnt, t0, itensors) = i->generate_task_list(tcnt, t0, gamma, itensors);
    out << tmp;
  }
  return make_tuple(out, tcnt, t0, itensors);
}

tuple<OutStream, int, int, vector<shared_ptr<Tensor>>>
  Tree::binarycontraction_generate_zero_ci(std::shared_ptr<BinaryContraction> j, int tcnt, int t0, const list<shared_ptr<Tensor>> gamma, vector<shared_ptr<Tensor>> itensors) const {
  OutStream out, tmp;
  vector<shared_ptr<Tensor>> source_tensors = j->tensors_vec();
  const bool diagonal = j->diagonal_only();

  num_ = tcnt;
  for (auto& s : source_tensors) {
    // if it contains a new intermediate tensor, dump a constructor
    if (find(itensors.begin(), itensors.end(), s) == itensors.end() && s->label().find("I") != string::npos) {
      itensors.push_back(s);
//      out.ee << s->constructor_str_ci(diagonal) << endl;
    }
  }
//  out.ee << " // in generate_task_ci" << endl;
//  out << generate_task_ci(num_, source_tensors, gamma, t0, diagonal);

  list<shared_ptr<const Index>> proj = j->target_index();
  // write out headers
  {
    list<shared_ptr<const Index>> ti = depth() != 0 ? j->target_indices() : proj;
//    out << generate_compute_header(num_, ti, source_tensors);
  }

  list<shared_ptr<const Index>> dm;
  if (proj.size() > 1) {
    for (auto i = proj.begin(); i != proj.end(); ++i, ++i) {
      auto k = i; ++k;
      dm.push_back(*k);
      dm.push_back(*i);
    }
  } else if (proj.size() == 1) {
    for (auto& i : proj) dm.push_back(i);
  } else {
    throw logic_error("Tree::binarycontraction_generate_zero, should not have empty target index here.");
  }

  // virtual
  shared_ptr<Tensor> proj_tensor = create_tensor(dm);

  vector<shared_ptr<Tensor>> op2 = { j->next_target() };
// blame 2
//  out << generate_compute_operators(proj_tensor, op2, j->dagger());

  {
    // send outer loop indices if outer loop indices exist, otherwise send inner indices
    list<shared_ptr<const Index>> ti = depth() != 0 ? j->target_indices() : proj;
    if (depth() == 0)
      if (ti.size() > 1) {
        for (auto m = ti.begin(), n = ++ti.begin(); m != ti.end(); ++m, ++m, ++n, ++n)
          swap(*m, *n);
      }
// blame 3
//    out << generate_compute_footer(num_, ti, source_tensors, false);
  }

  ++tcnt;

  return make_tuple(out, tcnt, t0, itensors);
}


tuple<OutStream, int, int, vector<shared_ptr<Tensor>>>
  Tree::binarycontraction_generate_zero(std::shared_ptr<BinaryContraction> j, int tcnt, int t0, const list<shared_ptr<Tensor>> gamma, vector<shared_ptr<Tensor>> itensors) const {
  OutStream out, tmp;
  vector<shared_ptr<Tensor>> source_tensors = j->tensors_vec();
  const bool diagonal = j->diagonal_only();

  num_ = tcnt;
  for (auto& s : source_tensors) {
    // if it contains a new intermediate tensor, dump a constructor
    if (find(itensors.begin(), itensors.end(), s) == itensors.end() && s->label().find("I") != string::npos) {
      itensors.push_back(s);
      out.ee << s->constructor_str(diagonal) << endl;
    }
  }
  out << generate_task(num_, source_tensors, gamma, t0, diagonal);

  list<shared_ptr<const Index>> proj = j->target_index();
  // write out headers
  {
    list<shared_ptr<const Index>> ti = depth() != 0 ? j->target_indices() : proj;
    // if outer loop is empty, send inner loop indices to header
    if (ti.size() == 0) {
      assert(depth() != 0);
      list<shared_ptr<const Index>> di = j->loop_indices();
      di.reverse();
      out << generate_compute_header(num_, di, source_tensors, true);

    } else {
      out << generate_compute_header(num_, ti, source_tensors);
    }
  }

  list<shared_ptr<const Index>> dm;
  if (proj.size() > 1) {
    for (auto i = proj.begin(); i != proj.end(); ++i, ++i) {
      auto k = i; ++k;
      dm.push_back(*k);
      dm.push_back(*i);
    }
  } else if (proj.size() == 1) {
    for (auto& i : proj) dm.push_back(i);
  } else {
    throw logic_error("Tree::binarycontraction_generate_zero, should not have empty target index here.");
  }

  // virtual
  shared_ptr<Tensor> proj_tensor = create_tensor(dm);

  vector<shared_ptr<Tensor>> op2 = { j->next_target() };
  out << generate_compute_operators(proj_tensor, op2, j->dagger());

  {
    // send outer loop indices if outer loop indices exist, otherwise send inner indices
    list<shared_ptr<const Index>> ti = depth() != 0 ? j->target_indices() : proj;
    if (depth() == 0)
      if (ti.size() > 1) {
        for (auto m = ti.begin(), n = ++ti.begin(); m != ti.end(); ++m, ++m, ++n, ++n)
          swap(*m, *n);
      }
    if (ti.size() == 0) {
      assert(depth() != 0);
      // sending inner indices
      list<shared_ptr<const Index>> di = j->loop_indices();
      out << generate_compute_footer(num_, di, source_tensors, true);
    } else {
      // sending outer indices
      out << generate_compute_footer(num_, ti, source_tensors, false);
    }
  }

  ++tcnt;

  return make_tuple(out, tcnt, t0, itensors);
}

tuple<OutStream, int, int, vector<shared_ptr<Tensor>>>
  Tree::generate_task_list_zero(int tcnt, int t0, const list<shared_ptr<Tensor>> gamma, vector<shared_ptr<Tensor>> itensors) const {
    OutStream out, tmp;

   // process tree with target indices eg, ci derivative, density matrix
   num_ = tcnt;
   // save density task zero
   t0 = tcnt;

   // virtual target
   bool cicontraction = (this->label().find("deci") != string::npos);
   if (cicontraction)
     out << create_target_ci(tcnt);
   else
     out << create_target(tcnt);
   ++tcnt;

   /////////////////////////////////////////////////////////////////
   // walk through BinaryContraction
   /////////////////////////////////////////////////////////////////
   for (auto& j : bc_) {
     // if at top bc, add a task to for top level contraction (proj)

     if (cicontraction)
       tie(tmp, tcnt, t0, itensors) = binarycontraction_generate_zero_ci(j, tcnt, t0, gamma, itensors);
     else
       tie(tmp, tcnt, t0, itensors) = binarycontraction_generate_zero(j, tcnt, t0, gamma, itensors);

     out << tmp;

     tie(tmp, tcnt, t0, itensors) = j->generate_task_list(tcnt, t0, gamma, itensors);
     out << tmp;

   }
  return make_tuple(out, tcnt, t0, itensors);
}

tuple<OutStream, int, int, vector<shared_ptr<Tensor>>>
  Tree::generate_task_list(int tcnt, int t0, const list<shared_ptr<Tensor>> gamma, vector<shared_ptr<Tensor>> itensors) const {
  // here ss is dependencies, tt is tasks
  OutStream out, tmp;
  string indent = "      ";

  if (depth() == 0) { //////////////////// zero depth /////////////////////////////
    if (root_targets()) {
      tie(tmp, tcnt, t0, itensors) = generate_task_list_zero(tcnt, t0, gamma, itensors);
      out << tmp;

    } else {  // trees without root target indices
      out.ee << "  auto " << label() << "q = make_shared<Queue>();" << endl;
      num_ = tcnt;
      for (auto& j : bc_) {
        tie(tmp, tcnt, t0, itensors) = j->generate_task_list(tcnt, t0, gamma, itensors);
        out << tmp;
      }
    }
  } else { //////////////////// non-zero depth /////////////////////////////
    tie(tmp, tcnt, t0, itensors) = generate_steps(indent, tcnt, t0, gamma, itensors);
    out << tmp;
  }

  return make_tuple(out, tcnt, t0, itensors);
}

tuple<OutStream, int, vector<shared_ptr<Tensor>>>
    Tree::binarycontraction_generate_gamma(shared_ptr<BinaryContraction> i, int tcnt, const list<shared_ptr<Tensor>> gamma, int t0, vector<shared_ptr<Tensor>> itensors) const {

  OutStream out;
  OutStream tmp;

  // Claim that we do CI contraction
//  out.ee << "// CI contraction, depth = " << depth() << endl;

  vector<shared_ptr<Tensor>> source_tensors = i->tensors_vec();

//  out.ee << "// Task" << tcnt << " ::: " << source_tensors[0]->label() << " = "
//    << source_tensors[1]->factor() * source_tensors[2]->factor() << " * " << source_tensors[1]->label()
//    << " * " << source_tensors[2]->label() << endl;
  list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->target_index();
  list<shared_ptr<const Index>> di = i->loop_indices();
//  out.ee << "// Contraction index: " << di.size() << " , ";
//  if (ti.size() != 0) {
//    for (auto iter = di.rbegin(); iter != di.rend(); ++iter) {
//      string index = (*iter)->str_gen();
//      out.ee << index << " ";
//    }
//  }
//  out.ee << endl;
  (source_tensors[1])->index().pop_back();

  const bool diagonal = i->diagonal_only();
  for (auto& s : source_tensors) {
    // if it contains a new intermediate tensor, dump a constructor -- somehow this does not work now
    if (find(itensors.begin(), itensors.end(), s) == itensors.end() && s->label().find("I") != string::npos) {
      itensors.push_back(s);
      out.ee << s->constructor_str(diagonal) << endl;
    }
  }
  // saving a counter to a protected member for dependency checks
  num_ = tcnt;
  bool merged = false;
  if (source_tensors[1]->merged()) merged = true;
  // if gamma, output is _0 ... _5. if rdm0deriv_, output is only _0
  if (source_tensors[1]->label().find("Gamma") != string::npos)
    out << generate_task_gamma(num_, source_tensors, gamma, t0, diagonal, true, merged);
  else
    out << generate_task_gamma(num_, source_tensors, gamma, t0, diagonal, false, merged);

  // use virtual function to generate a task for this binary contraction
  bool use_blas = false;
  if (source_tensors[1]->label().find("Gamma") != string::npos) {
    // we remove "ci0" index, and go for generate_gamma_sources to make the merged task
    out << (source_tensors[1])->generate_gamma_sources(num_, use_blas, true, source_tensors[2], di);
  } else {
    // it becomes rdm0deriv_, which takes extremely simple form
    list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->target_index();
    out << generate_bc_sources(num_, ti, source_tensors, false, false, i);
  }

  // increment tcnt before going to subtrees
  ++tcnt;
  // triggers a recursive call
  tie(tmp, tcnt, t0, itensors) = i->generate_task_list(tcnt, t0, gamma, itensors);
  out << tmp;

  return make_tuple(out, tcnt, itensors);
}


tuple<OutStream, int, vector<shared_ptr<Tensor>>>
    Tree::binarycontraction_generate(shared_ptr<BinaryContraction> i, int tcnt, const list<shared_ptr<Tensor>> gamma, int t0, vector<shared_ptr<Tensor>> itensors) const {
  OutStream out;
  OutStream tmp;

  vector<shared_ptr<Tensor>> source_tensors = i->tensors_vec();

//  if (depth() != 0) {
//    out.ee << "// Task" << tcnt << " ::: " << source_tensors[0]->label() << " = "
//      << source_tensors[1]->factor() * source_tensors[2]->factor() << " * " << source_tensors[1]->label()
//      << " * " << source_tensors[2]->label() << endl;
//    out.ee << "// Contractions: ";
//    list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->target_index();
//    list<shared_ptr<const Index>> di = i->loop_indices();
//    if (ti.size() != 0) {
//      for (auto iter = di.rbegin(); iter != di.rend(); ++iter) {
//        string index = (*iter)->str_gen();
//        out.ee << index;
//      }
//    }
//    out.ee << endl;
//  }

  const bool diagonal = i->diagonal_only();
  for (auto& s : source_tensors) {
    // if it contains a new intermediate tensor, dump a constructor -- somehow this does not work now
    if (find(itensors.begin(), itensors.end(), s) == itensors.end() && s->label().find("I") != string::npos) {
      itensors.push_back(s);
      out.ee << s->constructor_str(diagonal) << endl;
    }
  }
  // saving a counter to a protected member for dependency checks
  num_ = tcnt;
  out << generate_task(num_, source_tensors, gamma, t0, diagonal);

  // write out headers
  {
    list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->target_index();
    // if outer loop is empty, send inner loop indices to header
    if (ti.size() == 0) {
      assert(depth() != 0);
      list<shared_ptr<const Index>> di = i->loop_indices();
      di.reverse();
      out << generate_compute_header(num_, di, source_tensors, true);
    } else {
      out << generate_compute_header(num_, ti, source_tensors);
    }
  }

  // use virtual function to generate a task for this binary contraction
  out << generate_bc(i);

  {
    // send outer loop indices if outer loop indices exist, otherwise send inner indices
    list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->target_index();
    if (depth() == 0)
      for (auto i = ti.begin(), j = ++ti.begin(); i != ti.end(); ++i, ++i, ++j, ++j)
        swap(*i, *j);
    if (ti.size() == 0) {
      assert(depth() != 0);
      // sending inner indices
      list<shared_ptr<const Index>> di = i->loop_indices();
      out << generate_compute_footer(num_, di, source_tensors, true);
    } else {
      // sending outer indices
      out << generate_compute_footer(num_, ti, source_tensors, false);
    }
  }
  ///////////////////////////////////////////////////////////////////////

  // increment tcnt before going to subtrees
  ++tcnt;
  // triggers a recursive call
  tie(tmp, tcnt, t0, itensors) = i->generate_task_list(tcnt, t0, gamma, itensors);
  out << tmp;

  return make_tuple(out, tcnt, itensors);
}


tuple<OutStream, int, int, vector<shared_ptr<Tensor>>>
    Tree::generate_steps(string indent, int tcnt, int t0, const list<shared_ptr<Tensor>> gamma, vector<shared_ptr<Tensor>> itensors) const {
  OutStream out, tmp;
  /////////////////////////////////////////////////////////////////
  // if op_ is not empty, we add a task that adds up op_.
  /////////////////////////////////////////////////////////////////
  if (!op_.empty()) {

    // step through operators and if they are new, construct them.
    if (find(itensors.begin(), itensors.end(), target_) == itensors.end()) {
      itensors.push_back(target_);
      out.ee << target_->constructor_str(diagonal_only()) << endl;
    }

    vector<shared_ptr<Tensor>> op = {target_};
    op.insert(op.end(), op_.begin(), op_.end());
    // check if op_ has gamma tensor
    bool diagonal = nogamma_upstream();
    for (auto& i : op_) {
      string label = i->label();
      diagonal &= label.find("Gamma") == string::npos;
    }
    out << generate_task(tcnt, op, gamma, t0, diagonal);

    list<shared_ptr<const Index>> ti = target_->index();

    // make sure no duplicates in tensor list for compute header & footer
    vector<shared_ptr<Tensor>> uniq_tensors;
    vector<string> tensor_labels;
    for (auto& i : op) {
      string label = label__(i->label());
      if (find(tensor_labels.begin(), tensor_labels.end(), label) != tensor_labels.end()) continue;
      tensor_labels.push_back(label);
      uniq_tensors.push_back(i);
    }

    out << generate_compute_header(tcnt, ti, uniq_tensors);
    out << generate_compute_operators(target_, op_);
    out << generate_compute_footer(tcnt, ti, uniq_tensors, false);

    ++tcnt;
  }

  /////////////////////////////////////////////////////////////////
  // step through BinaryContraction
  /////////////////////////////////////////////////////////////////
  for (auto& i : bc_) {
    vector<shared_ptr<Tensor>> source_tensors = i->tensors_vec();

    bool cicontraction = (((source_tensors[1]->label().find("Gamma") != string::npos) || (source_tensors[1]->label().find("rdm0") != string::npos))
        && (this->label().find("deci") != string::npos));

    if (cicontraction)
      tie(tmp, tcnt, itensors) = binarycontraction_generate_gamma(i, tcnt, gamma, t0, itensors);
    else
      tie(tmp, tcnt, itensors) = binarycontraction_generate(i, tcnt, gamma, t0, itensors);

    out << tmp;
  }

  return make_tuple(out, tcnt, t0, itensors);
}


vector<shared_ptr<Tensor>> BinaryContraction::tensors_vec() {
  vector<shared_ptr<Tensor>> out;
  if (target_) out.push_back(target_);
  out.push_back(tensor_);
  if (!subtree_.empty())
    out.push_back(subtree_.front()->target());
  else if (source_)
    out.push_back(source_);

  return out;
}


bool BinaryContraction::diagonal_only() const {
  return tensor_->label().find("Gamma") == string::npos
      && all_of(subtree_.begin(), subtree_.end(), [](shared_ptr<Tree> i){ return i->diagonal_only(); })
      && (!source_ || source_->label().find("Gamma") == string::npos)
      && nogamma_upstream();
}


bool BinaryContraction::nogamma_upstream() const {
  return tensor_->label().find("Gamma") == std::string::npos
      && parent_->nogamma_upstream();
}


vector<string> BinaryContraction::required_rdm(vector<string> orig) const {
  vector<string> out = orig;
  for (auto& i : subtree_)
    out = i->required_rdm(out);

  sort(out.begin(), out.end());
  return out;
}

vector<string> Tree::required_rdm(vector<string> orig) const {
  vector<string> out = orig;
  for (auto& i : bc_)
    out = i->required_rdm(out);

  for (auto& i : op_) {
    if (i->label().find("Gamma") == string::npos) continue;
    vector<string> rdmn = i->active()->required_rdm();
    for (auto& j: rdmn)
      if (find(out.begin(), out.end(), j) == out.end()) out.push_back(j);
  }
  sort(out.begin(), out.end());
  return out;
}



