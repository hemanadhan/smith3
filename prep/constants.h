//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: constants.h
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

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#include <atomic>
#include <sstream>
#include <string>

namespace SMITH3 {
namespace Prep {

static std::string header() {
  std::stringstream mm;
  mm << "//" << std::endl;
  mm << "// SMITH3 - generates spin-free multireference electron correlation programs." << std::endl;
  mm << "// Filename: main.cc" << std::endl;
  mm << "// Copyright (C) 2012 Toru Shiozaki" << std::endl;
  mm << "//" << std::endl;
  mm << "// Author: Toru Shiozaki <shiozaki@northwestern.edu>" << std::endl;
  mm << "// Maintainer: Shiozaki group" << std::endl;
  mm << "//" << std::endl;
  mm << "// This file is part of the SMITH3 package." << std::endl;
  mm << "//" << std::endl;
  mm << "// The SMITH3 package is free software; you can redistribute it and/or modify" << std::endl;
  mm << "// it under the terms of the GNU Library General Public License as published by" << std::endl;
  mm << "// the Free Software Foundation; either version 2, or (at your option)" << std::endl;
  mm << "// any later version." << std::endl;
  mm << "//" << std::endl;
  mm << "// The SMITH3 package is distributed in the hope that it will be useful," << std::endl;
  mm << "// but WITHOUT ANY WARRANTY; without even the implied warranty of" << std::endl;
  mm << "// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" << std::endl;
  mm << "// GNU Library General Public License for more details." << std::endl;
  mm << "//" << std::endl;
  mm << "// You should have received a copy of the GNU Library General Public License" << std::endl;
  mm << "// along with the SMITH3 package; see COPYING.  If not, write to" << std::endl;
  mm << "// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA." << std::endl;
  mm << "//" << std::endl;
  mm << "" << std::endl;
  mm << "" << std::endl;
  mm << "// This program is supposed to perform Wick's theorem for multireference problems." << std::endl;
  mm << "// Spin averaged quantities assumed." << std::endl;
  mm << "" << std::endl;
  mm << "#include <fstream>" << std::endl;
  mm << "#include \"constants.h\"" << std::endl;
  mm << "#include \"forest.h\"" << std::endl;
  mm << "#include \"residual.h\"" << std::endl;
  mm << "" << std::endl;
  mm << "using namespace std;" << std::endl;
  mm << "using namespace smith;" << std::endl;
  mm << "" << std::endl;
  mm << "int main() {" << std::endl;
  return mm.str();
}

static std::string footer(const std::string res, const std::string energy = "", const std::string correction = "",
                          const std::string density = "", const std::string density1 = "", const std::string density2 = "",
                          const std::string dedci = "", const std::string dedci2 = "", const std::string dedci3 = "", const std::string dedci4 = "",
                          const std::string source = "", const std::string norm = "") {
  std::stringstream mm;

  mm << "  list<shared_ptr<Tree>> trees = {";
  std::atomic_flag done;
  done.clear();
  if (!res.empty())        mm << (done.test_and_set() ? ", " : "") << res;
  if (!source.empty())     mm << (done.test_and_set() ? ", " : "") << source;
  if (!energy.empty())     mm << (done.test_and_set() ? ", " : "") << energy;
  if (!correction.empty()) mm << (done.test_and_set() ? ", " : "") << correction;
  if (!density.empty())    mm << (done.test_and_set() ? ", " : "") << density;
  if (!density1.empty())   mm << (done.test_and_set() ? ", " : "") << density1;
  if (!density2.empty())   mm << (done.test_and_set() ? ", " : "") << density2;
  if (!dedci.empty())      mm << (done.test_and_set() ? ", " : "") << dedci;
  if (!dedci2.empty())     mm << (done.test_and_set() ? ", " : "") << dedci2;
  if (!dedci3.empty())     mm << (done.test_and_set() ? ", " : "") << dedci3;
  if (!dedci4.empty())     mm << (done.test_and_set() ? ", " : "") << dedci4;
  if (!norm.empty())       mm << (done.test_and_set() ? ", " : "") << norm;
  mm <<  "};" << std::endl;
  mm << "  auto fr = make_shared<Forest>(trees);" << std::endl;

  mm << "" <<  std::endl;
  mm << "  fr->filter_gamma();" << std::endl;
  mm << "  list<shared_ptr<Tensor>> gm = fr->gamma();" << std::endl;
  mm << "  const list<shared_ptr<Tensor>> gamma = gm;" << std::endl;

  mm << "" <<  std::endl;
  mm << "  auto tmp = fr->generate_code();" << std::endl;

  mm << "" <<  std::endl;
  mm << "  ofstream fs(fr->name() + \".h\");" << std::endl;
  mm << "  ofstream es(fr->name() + \"_tasks.h\");" << std::endl;
  mm << "  ofstream cs(fr->name() + \"_gen.cc\");" << std::endl;
  mm << "  ofstream ds(fr->name() + \"_tasks.cc\");" << std::endl;
  mm << "  ofstream gs(fr->name() + \".cc\");" << std::endl;
  mm << "  ofstream gg(fr->name() + \"_gamma.cc\");" << std::endl;
  mm << "  fs << tmp.ss.str();" << std::endl;
  mm << "  es << tmp.tt.str();" << std::endl;
  mm << "  cs << tmp.cc.str();" << std::endl;
  mm << "  ds << tmp.dd.str();" << std::endl;
  mm << "  gs << tmp.ee.str();" << std::endl;
  mm << "  gg << tmp.gg.str();" << std::endl;
  mm << "  fs.close();" << std::endl;
  mm << "  es.close();" << std::endl;
  mm << "  cs.close();" << std::endl;
  mm << "  ds.close();" << std::endl;
  mm << "  gs.close();" << std::endl;
  mm << "  gg.close();" << std::endl;
  mm << "  cout << std::endl;" << std::endl;
  mm << "" <<  std::endl;
  mm << "  // output" << std::endl;
  if (!res.empty()) {
    mm << "  cout << std::endl << \"   ***  Residual  ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << res << "->print();" << std::endl;
  }
  if (!source.empty()) {
    mm << "  cout << std::endl << \"   ***  Source  ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << source << "->print();" << std::endl;
  }

  if (!energy.empty()) {
    mm << "  cout << std::endl << \"   ***  Energy E2 ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << energy << "->print();" << std::endl;
  }
  if (!correction.empty()) {
    mm << "  cout << std::endl << \"   ***  Correlated norm <1|1> ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << correction << "->print();" << std::endl;
  }
  if (!norm.empty()) {
    mm << "  cout << std::endl << \"   ***  Norm <omega|1> ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << norm << "->print();" << std::endl;
  }
  if (!density.empty()) {
    mm << "  cout << std::endl << \"   ***  Correlated one-body density matrix d2 ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << density << "->print();" << std::endl;
  }
  if (!density1.empty()) {
    mm << "  cout << std::endl << \"   ***  Correlated one-body density matrix d1 ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << density1 << "->print();" << std::endl;
  }
  if (!density2.empty()) {
    mm << "  cout << std::endl << \"   ***  Correlated two-body density matrix D1 ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << density2 << "->print();" << std::endl;
  }
  if (!dedci.empty()) {
    mm << "  cout << std::endl << \"   ***  CI derivative  ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << dedci << "->print();" << std::endl;
  }
  if (!dedci2.empty()) {
    mm << "  cout << std::endl << \"   ***  CI derivative 2 ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << dedci2 << "->print();" << std::endl;
  }
  if (!dedci3.empty()) {
    mm << "  cout << std::endl << \"   ***  CI derivative 3 ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << dedci3 << "->print();" << std::endl;
  }
  if (!dedci4.empty()) {
    mm << "  cout << std::endl << \"   ***  CI derivative 4 ***\" << std::endl << std::endl;" << std::endl;
    mm << "  " << dedci4 << "->print();" << std::endl;
  }
  mm << "  cout << std::endl << std::endl;" << std::endl;
  mm << "" <<  std::endl;
  mm << "  return 0;" << std::endl;
  mm << "}" << std::endl;
  return mm.str();
}

static std::string footer_ci(const std::string res, const std::string source, const std::string norm) {
  return footer(res, "", "", "", "", "", "", "", "", "", source, norm);
}


}}

#endif
