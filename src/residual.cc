//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: residual.cc
// Copyright (C) 2013 Matthew MacLeod
//
// Author: Matthew MacLeod <matthew.macleod@northwestern.edu>
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


#include <iomanip>
#include "constants.h"
#include "residual.h"

using namespace std;
using namespace smith;


OutStream Residual::create_target(const int i) const {
  OutStream out;

  out.tt << "class Task" << i << " : public Task {" << endl;
  out.tt << "  protected:" << endl;
  out.tt << "    std::shared_ptr<Tensor> " << target_name__(label_) << "_;" << endl;
  out.tt << "    IndexRange closed_;" << endl;
  out.tt << "    IndexRange active_;" << endl;
  out.tt << "    IndexRange virt_;" << endl;
  out.tt << "    const bool reset_;" << endl;
  out.tt << "" << endl;
  out.tt << "    void compute_() {" << endl;
  out.tt << "      if (reset_) " << target_name__(label_) << "_->zero();" << endl;
  out.tt << "    }" << endl;
  out.tt << "" << endl;
  out.tt << "  public:" << endl;
  out.tt << "    Task" << i << "(std::vector<std::shared_ptr<Tensor>> t, const bool reset);" << endl;

  out.cc << "Task" << i << "::Task" << i << "(vector<shared_ptr<Tensor>> t, const bool reset) : reset_(reset) {" << endl;
  out.cc << "  " << target_name__(label_) << "_ =  t[0];" << endl;
  out.cc << "}" << endl << endl << endl;

  out.tt << "    ~Task" << i << "() {}" << endl;
  out.tt << "};" << endl << endl;

  out.ee << "  auto " << label_ << "q = make_shared<Queue>();" << endl;
  out.ee << "  auto tensor" << i << " = vector<shared_ptr<Tensor>>{" << target_name__(label_) << "};" << endl;
  out.ee << "  auto task" << i << " = make_shared<Task" << i << ">(tensor" << i << ", reset);" << endl;
  out.ee << "  " << label_ << "q->add_task(task" << i << ");" << endl << endl;

  return out;
}

OutStream Residual::create_target_ci(const int i) const {
  OutStream out;

  out.tt << "class Task" << i << " : public Task {" << endl;
  out.tt << "  protected:" << endl;
  out.tt << "    std::shared_ptr<Tensor> den0ci_;" << endl;
  out.tt << "    std::shared_ptr<Tensor> den1ci_;" << endl;
  out.tt << "    std::shared_ptr<Tensor> den2ci_;" << endl;
  out.tt << "    std::shared_ptr<Tensor> den3ci_;" << endl;
  out.tt << "    std::shared_ptr<Tensor> den4ci_;" << endl;
  out.tt << "    IndexRange closed_;" << endl;
  out.tt << "    IndexRange active_;" << endl;
  out.tt << "    IndexRange virt_;" << endl;
  out.tt << "    const bool reset_;" << endl;
  out.tt << "" << endl;
  out.tt << "    void compute_() {" << endl;
  out.tt << "      if (reset_) {" << endl;
  out.tt << "        den0ci_->zero();" << endl;
  out.tt << "        den1ci_->zero();" << endl;
  out.tt << "        den2ci_->zero();" << endl;
  out.tt << "        den3ci_->zero();" << endl;
  out.tt << "        den4ci_->zero();" << endl;
  out.tt << "      }" << endl;
  out.tt << "    }" << endl;
  out.tt << "" << endl;
  out.tt << "  public:" << endl;
  out.tt << "    Task" << i << "(std::vector<std::shared_ptr<Tensor>> t, const bool reset);" << endl;

  out.cc << "Task" << i << "::Task" << i << "(vector<shared_ptr<Tensor>> t, const bool reset) : reset_(reset) {" << endl;
  out.cc << "  den0ci_ =  t[0];" << endl;
  out.cc << "  den1ci_ =  t[1];" << endl;
  out.cc << "  den2ci_ =  t[2];" << endl;
  out.cc << "  den3ci_ =  t[3];" << endl;
  out.cc << "  den4ci_ =  t[4];" << endl;
  out.cc << "}" << endl << endl << endl;

  out.tt << "    ~Task" << i << "() {}" << endl;
  out.tt << "};" << endl << endl;

  out.ee << "  auto " << label_ << "q = make_shared<Queue>();" << endl;
  out.ee << "  auto tensor" << i << " = vector<shared_ptr<Tensor>>{den0ci, den1ci, den2ci, den3ci, den4ci};" << endl;
  out.ee << "  auto task" << i << " = make_shared<Task" << i << ">(tensor" << i << ", reset);" << endl;
  out.ee << "  " << label_ << "q->add_task(task" << i << ");" << endl << endl;

  return out;
}


OutStream Residual::generate_task_gamma(const int ip, const int ic, const vector<string> op, const string scalar, const int i0, bool der, bool diagonal) const {
  stringstream tmp;

  // when there is no gamma under this, we must skip for off-digonal
  string indent = "";

  if (diagonal) {
    tmp << "  shared_ptr<Task" << ic << "> task" << ic << ";" << endl;
    tmp << "  if (diagonal) {" << endl;
    indent += "  ";
  }
  const bool is_gamma = op.front().find("Gamma") != string::npos;
  tmp << indent << "  auto tensor" << ic << " = vector<shared_ptr<Tensor>>{" << merge__(op, label_) << "};" << endl;
  tmp << indent << "  " << (diagonal ? "" : "auto ") << "task" << ic
                << " = make_shared<Task" << ic << ">(tensor" << ic << ", pindex"
                << (scalar.empty() ? "" : ", this->e0_") << ");" << endl;

  if (!is_gamma) {
    if (parent_) {
      assert(parent_->parent());
//      tmp << indent << "  task" << ip << "->add_dep(task" << ic << ");" << endl;
      tmp << indent << "  task" << ic << "->add_dep(task" << i0 << ");" << endl;
    } else {
      assert(depth() == 0);
      tmp << indent << "  task" << ic << "->add_dep(task" << i0 << ");" << endl;
    }
    tmp << indent << "  " << label_ << "q->add_task(task" << ic << ");" << endl;
  }
  if (diagonal)
    tmp << "  }" << endl;

  if (!is_gamma)
    tmp << endl;

  OutStream out;
  if (!is_gamma) {
    out.ee << tmp.str();
  } else {
    out.gg << tmp.str();
  }
  return out;
}




OutStream Residual::generate_task(const int ip, const int ic, const vector<string> op, const string scalar, const int i0, bool der, bool diagonal) const {
  stringstream tmp;

  // when there is no gamma under this, we must skip for off-digonal
  string indent = "";

  if (diagonal) {
    tmp << "  shared_ptr<Task" << ic << "> task" << ic << ";" << endl;
    tmp << "  if (diagonal) {" << endl;
    indent += "  ";
  }
  const bool is_gamma = op.front().find("Gamma") != string::npos;
  tmp << indent << "  auto tensor" << ic << " = vector<shared_ptr<Tensor>>{" << merge__(op, label_) << "};" << endl;
  tmp << indent << "  " << (diagonal ? "" : "auto ") << "task" << ic
                << " = make_shared<Task" << ic << ">(tensor" << ic << ", pindex"
                << (scalar.empty() ? "" : ", this->e0_") << ");" << endl;

  if (!is_gamma) {
    if (parent_) {
      assert(parent_->parent());
      tmp << indent << "  task" << ip << "->add_dep(task" << ic << ");" << endl;
      tmp << indent << "  task" << ic << "->add_dep(task" << i0 << ");" << endl;
    } else {
      assert(depth() == 0);
      tmp << indent << "  task" << ic << "->add_dep(task" << i0 << ");" << endl;
    }
    tmp << indent << "  " << label_ << "q->add_task(task" << ic << ");" << endl;
  }
  if (diagonal)
    tmp << "  }" << endl;

  if (!is_gamma)
    tmp << endl;

  OutStream out;
  if (!is_gamma) {
    out.ee << tmp.str();
  } else {
    out.gg << tmp.str();
  }
  return out;
}


OutStream Residual::generate_compute_header(const int ic, const list<shared_ptr<const Index>> ti, const vector<shared_ptr<Tensor>> tensors, const bool no_outside) const {
  vector<string> labels;
  for (auto i = ++tensors.begin(); i != tensors.end(); ++i)
    labels.push_back((*i)->label());
  const int ninptensors = count_distinct_tensors__(labels);

  bool need_e0 = false;
  for (auto& s : tensors)
    if (!s->scalar().empty()) need_e0 = true;

  const int nindex = ti.size();
  OutStream out;
  out.tt << "class Task" << ic << " : public Task {" << endl;
  out.tt << "  protected:" << endl;
  out.tt << "    std::shared_ptr<Tensor> out_;" << endl;
  out.tt << "    std::array<std::shared_ptr<const Tensor>," << ninptensors << "> in_;" << endl;
  // if index is empty give dummy arg
  out.tt << "    class Task_local : public SubTask<" << (ti.empty() ? 1 : nindex) << "," << ninptensors << "> {" << endl;
  out.tt << "      protected:" << endl;
  out.tt << "        const std::array<std::shared_ptr<const IndexRange>,3> range_;" << endl << endl;

  out.tt << "        const Index& b(const size_t& i) const { return this->block(i); }" << endl;
  out.tt << "        std::shared_ptr<const Tensor> in(const size_t& i) const { return this->in_tensor(i); }" << endl;
  out.tt << "        std::shared_ptr<Tensor> out() { return this->out_tensor(); }" << endl;
  if (need_e0)
    out.tt << "        const double e0_;" << endl;
  out.tt << endl;

  out.tt << "      public:" << endl;
  // if index is empty use dummy index 1 to subtask
  if (ti.empty()) {
    out.tt << "        Task_local(const std::array<std::shared_ptr<const Tensor>," << ninptensors <<  ">& in, std::shared_ptr<Tensor>& out," << endl;
    out.tt << "                   std::array<std::shared_ptr<const IndexRange>,3>& ran" << (need_e0 ? ", const double e" : "") << ")" << endl;
    out.tt << "          : SubTask<1," << ninptensors << ">(std::array<const Index, 1>(), in, out), range_(ran)" << (need_e0 ? ", e0_(e)" : "") << " { }" << endl;
  } else {
    out.tt << "        Task_local(const std::array<const Index," << nindex << ">& block, const std::array<std::shared_ptr<const Tensor>," << ninptensors <<  ">& in, std::shared_ptr<Tensor>& out," << endl;
    out.tt << "                   std::array<std::shared_ptr<const IndexRange>,3>& ran" << (need_e0 ? ", const double e" : "") << ")" << endl;
    out.tt << "          : SubTask<" << nindex << "," << ninptensors << ">(block, in, out), range_(ran)" << (need_e0 ? ", e0_(e)" : "") << " { }" << endl;
  }
  out.tt << endl;
  out.tt << "        void compute() override;" << endl;

  out.dd << "void Task" << ic << "::Task_local::compute() {" << endl;

  if (!no_outside) {
    list<shared_ptr<const Index>> ti_copy = ti;
    if (depth() == 0) {
      if (ti.size() > 1)
        for (auto i = ti_copy.begin(), j = ++ti_copy.begin(); i != ti_copy.end(); ++i, ++i, ++j, ++j)
          swap(*i, *j);
    }

    int cnt = 0;
    for (auto i = ti_copy.rbegin(); i != ti_copy.rend(); ++i)
      out.dd << "  const Index " << (*i)->str_gen() << " = b(" << cnt++ << ");" << endl;
    out.dd << endl;
  }

  return out;
}


OutStream Residual::generate_compute_footer(const int ic, const list<shared_ptr<const Index>> ti, const vector<shared_ptr<Tensor>> tensors, const bool dot) const {
  vector<string> labels;
  for (auto i = ++tensors.begin(); i != tensors.end(); ++i)
    labels.push_back((*i)->label());
  const int ninptensors = count_distinct_tensors__(labels);
  assert(ninptensors > 0);

  bool need_e0 = false;
  for (auto& s : tensors)
    if (!s->scalar().empty()) need_e0 = true;

  OutStream out;
  out.dd << "}" << endl << endl << endl;

  out.tt << "    };" << endl;
  out.tt << "" << endl;
  out.tt << "    std::vector<std::shared_ptr<Task_local>> subtasks_;" << endl;
  out.tt << "" << endl;

  out.tt << "    void compute_() override {" << endl;
  out.tt << "      if (!out_->allocated())" << endl;
  out.tt << "        out_->allocate();" << endl;
  out.tt << "      for (auto& i : in_)" << endl;
  out.tt << "        i->init();" << endl;
  out.tt << "      for (auto& i : subtasks_) i->compute();" << endl;
  out.tt << "    }" << endl << endl;

  out.tt << "  public:" << endl;
  out.tt << "    Task" << ic << "(std::vector<std::shared_ptr<Tensor>> t, std::array<std::shared_ptr<const IndexRange>,3> range" << (need_e0 ? ", const double e" : "") << ");" << endl;

  out.cc << "Task" << ic << "::Task" << ic << "(vector<shared_ptr<Tensor>> t, array<shared_ptr<const IndexRange>,3> range" << (need_e0 ? ", const double e" : "") << ") {" << endl;
  out.cc << "  array<shared_ptr<const Tensor>," << ninptensors << "> in = {{";
  for (auto i = 1; i < ninptensors + 1; ++i)
    out.cc << "t[" << i << "]" << (i < ninptensors ? ", " : "");
  out.cc << "}};" << endl;

  out.cc << "  out_ = t[0];" << endl;
  out.cc << "  in_ = in;" << endl << endl;

  // over original outermost indices
  if (!ti.empty()) {
    out.cc << "  subtasks_.reserve(";
    for (auto i = ti.begin(); i != ti.end(); ++i) {
      if (i != ti.begin()) out.cc << "*";
      out.cc << (*i)->generate_range() << "->nblock()";
    }
    out.cc << ");" << endl;
  }
  // loops
  string indent = "  ";
  for (auto i = ti.begin(); i != ti.end(); ++i, indent += "  ")
    out.cc << indent << "for (auto& " << (*i)->str_gen() << " : *" << (*i)->generate_range() << ")" << endl;
  // parallel if
  string listind = "";
  for (auto i = ti.rbegin(); i != ti.rend(); ++i) {
    if (i != ti.rbegin()) listind += ", ";
    listind += (*i)->str_gen();
  }
  // if t[1] in the dot case is t2dagger, we further need to reverse the order
  string listind2 = "";
  if (dot && tensors[1]->label().find("dagger") != string::npos) {
    for (auto i = ti.begin(); i != ti.end(); ++i) {
      if (i != ti.begin()) listind2 += ", ";
      listind2 += (*i)->str_gen();
    }
  } else {
    listind2 = listind;
  }
  out.cc << indent << "if (t[" << (dot ? 1 : 0) << "]->is_local("<< listind2 << "))" << endl;
  indent += "  ";
  // add subtasks
  if (!ti.empty()) {
    out.cc << indent  << "subtasks_.push_back(make_shared<Task_local>(array<const Index," << ti.size() << ">{{" << listind;
    out.cc << "}}, in, t[0], range" << (need_e0 ? ", e" : "") << "));" << endl;
  } else {
    out.cc << indent  << "subtasks_.push_back(make_shared<Task_local>(in, t[0], range" << (need_e0 ? ", e" : "") << "));" << endl;
  }
  out.cc << "}" << endl << endl << endl;

  out.tt << "    ~Task" << ic << "() {}" << endl;
  out.tt << "};" << endl << endl;
  return out;
}


OutStream Residual::generate_bc(const shared_ptr<BinaryContraction> i) const {
  OutStream out;
  if (depth() != 0) {
    const string bindent = "  ";
    string dindent = bindent;

    out.dd << target_->generate_get_block(dindent, "o", "out()", true);
    out.dd << target_->generate_scratch_area(dindent, "o", "out()", true); // true means zero-out

    list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->tensor()->index();

    // inner loop will show up here
    // but only if outer loop is not empty
    list<shared_ptr<const Index>> di = i->loop_indices();
    vector<string> close2;
    vector<string> close3;
    string inlabel("in("); inlabel += (same_tensor__(i->tensor()->label(), i->next_target()->label()) ? "0)" : "1)");
    const constexpr size_t buffersize = 5;
    if (ti.size() != 0) {
      out.dd << endl;
      if (!di.empty()) {
        string dindent2 = dindent;
        out.dd << "#ifdef SMITH_NON_BLOCKING" << endl;
        out.dd << dindent << "auto loop = LoopGenerator::gen({";
        for (auto iter = di.rbegin(); iter != di.rend(); ++iter)
          out.dd << (iter != di.rbegin() ? ", " : "") << "*" << (*iter)->generate_range("_");
        out.dd << "});" << endl;
        out.dd << dindent << "const size_t loopsize = loop.size();" << endl;
        out.dd << dindent << "list<shared_ptr<RMATask<double>>> r0data;" << endl;
        out.dd << dindent << "list<shared_ptr<RMATask<double>>> r1data;" << endl;
        out.dd << dindent << "for (size_t i = 0; i != min(static_cast<size_t>(" << buffersize << "), loopsize); ++i) {" << endl; 
        int cnt = 0;
        for (auto iter = di.rbegin(); iter != di.rend(); ++iter, ++cnt)
          out.dd << dindent << "  auto& " << (*iter)->str_gen() << " = loop[i][" << cnt << "];" << endl;
        out.dd << i->tensor()->generate_get_block_nb(dindent + "  ", "r0", "in(0)");
        out.dd << i->next_target()->generate_get_block_nb(dindent + "  ", "r1", inlabel);
        out.dd << dindent << "}" << endl;
        out.dd << dindent << "for (size_t l = 0; l < loopsize; ++l) {" << endl;
        close2.push_back(dindent + "}");
        dindent += "  ";
        cnt = 0;
        for (auto iter = di.rbegin(); iter != di.rend(); ++iter, ++cnt)
          out.dd << dindent << "auto& " << (*iter)->str_gen() << " = loop[l][" << cnt << "];" << endl;
        out.dd << dindent << "r0data.front()->wait();" << endl;
        out.dd << dindent << "r1data.front()->wait();" << endl;
        out.dd << dindent << "std::unique_ptr<double[]> i0data = r0data.front()->move_buf();" << endl;
        out.dd << dindent << "std::unique_ptr<double[]> i1data = r1data.front()->move_buf();" << endl;
        out.dd << dindent << "r0data.pop_front();" << endl;
        out.dd << dindent << "r1data.pop_front();" << endl;
        out.dd << dindent << "if (l+" << buffersize << " < loopsize) {" << endl;
        cnt = 0;
        for (auto iter = di.rbegin(); iter != di.rend(); ++iter, ++cnt)
          out.dd << dindent << "  auto& " << (*iter)->str_gen() << " = loop[l+" << buffersize << "][" << cnt << "];" << endl;
        out.dd << i->tensor()->generate_get_block_nb(dindent + "  ", "r0", "in(0)");
        out.dd << i->next_target()->generate_get_block_nb(dindent + "  ", "r1", inlabel);
        out.dd << dindent << "}" << endl;
        out.dd << "#else" << endl;
        for (auto iter = di.rbegin(); iter != di.rend(); ++iter, dindent2 += "  ") {
          string index = (*iter)->str_gen();
          out.dd << dindent2 << "for (auto& " << index << " : *" << (*iter)->generate_range("_") << ") {" << endl;
          close3.push_back(dindent2 + "}");
        }
        out.dd << i->tensor()->generate_get_block(dindent2, "i0", "in(0)", false, /*noscale*/true);
        out.dd << i->next_target()->generate_get_block(dindent2, "i1", inlabel, false, /*noscale*/true);
        out.dd << "#endif" << endl;
      } else {
        out.dd << i->tensor()->generate_get_block(dindent, "i0", "in(0)", false, /*noscale*/true);
        out.dd << i->next_target()->generate_get_block(dindent, "i1", inlabel, false, /*noscale*/true);
      }
    } else {
      int cnt = 0;
      for (auto k = di.rbegin(); k != di.rend(); ++k, cnt++)
        out.dd << dindent << "const Index " <<  (*k)->str_gen() << " = b(" << cnt << ");" << endl;
      out.dd << endl;
      out.dd << i->tensor()->generate_get_block(dindent, "i0", "in(0)", false, /*noscale*/true);
      out.dd << i->next_target()->generate_get_block(dindent, "i1", inlabel, false, /*noscale*/true);
    }

    // retrieving tensor_
    out.dd << i->tensor()->generate_sort_indices(dindent, "i0", "in(0)", di, false, true) << endl;
    // retrieving subtree_
    out.dd << i->next_target()->generate_sort_indices(dindent, "i1", inlabel, di, false, true) << endl;

    // call dgemm or ddot (if only vector - vector contraction is made)
    {
      pair<string, string> t0 = i->tensor()->generate_dim(di);
      pair<string, string> t1 = i->next_target()->generate_dim(di);
      if (t0.first != "" || t1.first != "") {
        out.dd << dindent << GEMM << "(\"T\", \"N\", ";
        string tt0 = t0.first == "" ? "1" : t0.first;
        string tt1 = t1.first == "" ? "1" : t1.first;
        string ss0 = t1.second== "" ? "1" : t1.second;
        out.dd << tt0 << ", " << tt1 << ", " << ss0 << "," << endl;
        out.dd << dindent << "       1.0, i0data_sorted, " << ss0 << ", i1data_sorted, " << ss0 << "," << endl
           << dindent << "       1.0, odata_sorted, " << tt0;
        out.dd << ");" << endl;
      } else {
        string ss0 = t1.second== "" ? "1" : t1.second;
        out.dd << dindent << "odata_sorted[0] += ddot_(" << ss0 << ", i0data_sorted, 1, i1data_sorted, 1);" << endl;
      }
    }

    if (ti.size() != 0) {
      out.dd << "#ifdef SMITH_NON_BLOCKING" << endl;
      for (auto iter = close2.rbegin(); iter != close2.rend(); ++iter)
        out.dd << *iter << endl;
      out.dd << "#else" << endl;
      for (auto iter = close3.rbegin(); iter != close3.rend(); ++iter)
        out.dd << *iter << endl;
      out.dd << "#endif" << endl;
      out.dd << endl;
    }
    // Inner loop ends here

    // sort buffer
    {
      out.dd << i->target()->generate_sort_indices_target(bindent, "o", di, i->tensor(), i->next_target());
    }
    // put buffer
    {
      string label = target_->label();
      // new interface requires indices for put_block
      out.dd << bindent << "out()->add_block(odata";
      list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->tensor()->index();
      for (auto i = ti.rbegin(); i != ti.rend(); ++i)
        out.dd << ", " << (*i)->str_gen();
      out.dd << ");" << endl;
    }
  } else {  // now at bc depth 0
    // making residual vector...
    list<shared_ptr<const Index>> proj = i->target_index();
    list<shared_ptr<const Index>> res;
    assert(!(proj.size() & 1));
    for (auto i = proj.begin(); i != proj.end(); ++i, ++i) {
      auto j = i; ++j;
      res.push_back(*j);
      res.push_back(*i);
    }
    auto residual = make_shared<Tensor>(1.0, target_name__(label_), res);
    vector<shared_ptr<Tensor>> op2 = { i->next_target() };
    out << generate_compute_operators(residual, op2, i->dagger());
  }


  return out;
}


OutStream Residual::generate_bc_sources(const int ic, const list<shared_ptr<const Index>> ti, const vector<shared_ptr<Tensor>> tensors, const bool no_outside, const bool dot, const shared_ptr<BinaryContraction> i) const {
  OutStream out;
  const string bindent = "  ";
  string dindent = bindent;

  out << generate_header_sources(ic, ti, tensors, no_outside);

  out.dd << target_->generate_get_block(dindent, "o", "out()", true, true, -1) << endl;

//  list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->tensor()->index();

  // inner loop will show up here
  // but only if outer loop is not empty
  list<shared_ptr<const Index>> di = i->loop_indices();

  // retrieving subtree_
  out.dd << i->next_target()->generate_get_block(dindent, "i0", "in(0)") << endl;

  {
    pair<string, string> t0 = i->tensor()->generate_dim(di);
    pair<string, string> t1 = i->next_target()->generate_dim(di);
    string ss0 = t1.second== "" ? "1" : t1.second;
    out.dd << dindent << "odata[0] += " << setprecision(1) << fixed << i->tensor()->factor() << " * i0data[0];" << endl;
  }

  // put buffer
  {
    string label = target_->label();
    // new interface requires indices for put_block
    out.dd << bindent << "out()->add_block(odata);" << endl;
  }

  out << generate_footer_sources(ic, ti, tensors, dot);

  return out;
}


shared_ptr<Tensor> Residual::create_tensor(list<shared_ptr<const Index>> dm) const {
 return make_shared<Tensor>(1.0, "r", dm);
}



OutStream Residual::generate_header_sources(const int ic, const list<shared_ptr<const Index>> ti, const vector<shared_ptr<Tensor>> tensors, const bool no_outside) const {
  vector<string> labels;
  for (auto i = ++tensors.begin(); i != tensors.end(); ++i)
    labels.push_back((*i)->label());
  const int ninptensors = count_distinct_tensors__(labels);

  bool need_e0 = false;
  for (auto& s : tensors)
    if (!s->scalar().empty()) need_e0 = true;

  const int nindex = ti.size();
  OutStream out;
  out.tt << "class Task" << ic << " : public Task {" << endl;
  out.tt << "  protected:" << endl;
  out.tt << "    std::shared_ptr<Tensor> out_;" << endl;
  out.tt << "    std::array<std::shared_ptr<const Tensor>,1> in_;" << endl;
  // if index is empty give dummy arg
  out.tt << "    class Task_local : public SubTask<" << (ti.empty() ? 1 : nindex) << ",1> {" << endl;
  out.tt << "      protected:" << endl;
  out.tt << "        const std::array<std::shared_ptr<const IndexRange>,3> range_;" << endl << endl;

  out.tt << "        const Index& b(const size_t& i) const { return this->block(i); }" << endl;
  out.tt << "        std::shared_ptr<const Tensor> in(const size_t& i) const { return this->in_tensor(i); }" << endl;
  out.tt << "        std::shared_ptr<Tensor> out() { return this->out_tensor(); }" << endl;
  if (need_e0)
    out.tt << "        const double e0_;" << endl;
  out.tt << endl;

  out.tt << "      public:" << endl;
  // if index is empty use dummy index 1 to subtask
  out.tt << "        Task_local(const std::array<std::shared_ptr<const Tensor>,1>& in, std::shared_ptr<Tensor>& out," << endl;
  out.tt << "                   std::array<std::shared_ptr<const IndexRange>,3>& ran" << (need_e0 ? ", const double e" : "") << ")" << endl;
  out.tt << "          : SubTask<1,1>(std::array<const Index, 1>(), in, out), range_(ran)" << (need_e0 ? ", e0_(e)" : "") << " { }" << endl;
  out.tt << endl;
  out.tt << "        void compute() override;" << endl;

  out.dd << "void Task" << ic << "::Task_local::compute() {" << endl;

  if (!no_outside) {
    list<shared_ptr<const Index>> ti_copy = ti;
    if (depth() == 0) {
      if (ti.size() > 1)
        for (auto i = ti_copy.begin(), j = ++ti_copy.begin(); i != ti_copy.end(); ++i, ++i, ++j, ++j)
          swap(*i, *j);
    }

    int cnt = 0;
  }

  return out;
}


OutStream Residual::generate_footer_sources(const int ic, const list<shared_ptr<const Index>> ti, const vector<shared_ptr<Tensor>> tensors, const bool dot) const {
  vector<string> labels;
  for (auto i = ++tensors.begin(); i != tensors.end(); ++i)
    labels.push_back((*i)->label());
  const int ninptensors = count_distinct_tensors__(labels);
  assert(ninptensors > 0);

  bool need_e0 = false;
  for (auto& s : tensors)
    if (!s->scalar().empty()) need_e0 = true;

  OutStream out;
  out.dd << "}" << endl << endl << endl;

  out.tt << "    };" << endl;
  out.tt << "" << endl;
  out.tt << "    std::vector<std::shared_ptr<Task_local>> subtasks_;" << endl;
  out.tt << "" << endl;

  out.tt << "    void compute_() override {" << endl;
  out.tt << "      if (!out_->allocated())" << endl;
  out.tt << "        out_->allocate();" << endl;
  out.tt << "      for (auto& i : in_)" << endl;
  out.tt << "        i->init();" << endl;
  out.tt << "      for (auto& i : subtasks_) i->compute();" << endl;
  out.tt << "    }" << endl << endl;

  out.tt << "  public:" << endl;
  out.tt << "    Task" << ic << "(std::vector<std::shared_ptr<Tensor>> t, std::array<std::shared_ptr<const IndexRange>,3> range" << (need_e0 ? ", const double e" : "") << ");" << endl;

  out.cc << "Task" << ic << "::Task" << ic << "(vector<shared_ptr<Tensor>> t, array<shared_ptr<const IndexRange>,3> range" << (need_e0 ? ", const double e" : "") << ") {" << endl;
  out.cc << "  array<shared_ptr<const Tensor>,1> in = {{t[1]}};" << endl;

  out.cc << "  out_ = t[0];" << endl;
  out.cc << "  in_ = in;" << endl << endl;

  out.cc << "  subtasks_.push_back(make_shared<Task_local>(in, t[0], range" << (need_e0 ? ", e" : "") << "));" << endl;
  out.cc << "}" << endl << endl << endl;

  out.tt << "    ~Task" << ic << "() {}" << endl;
  out.tt << "};" << endl << endl;
  return out;
}


