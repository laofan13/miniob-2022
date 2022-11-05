/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2021/6/10.
//

#pragma once

#include "sql/parser/parse.h"
#include "sql/operator/operator.h"
#include "rc.h"

class JoinUnit;

// TODO fixme
class JoinOperator : public Operator
{
public:
  JoinOperator(Operator *left, Operator *right, JoinUnit *join_unit):
  left_(left), right_(right), join_unit_(join_unit)
  {
    total_num_ = 0;
    current_index_ = 0;
  }

  ~JoinOperator(){
    if(left_) {
      delete left_;
      left_ = nullptr;
    }
    if(right_) {
      delete right_;
      right_ = nullptr;
    }
    for(Tuple* tuple: composite_table_tuples_) {
        delete tuple;
    }
    composite_table_tuples_.clear();
  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override;
  //int tuple_cell_num() const override;
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override;
private:
  bool do_predicate(Tuple *tuple);

private:
  Operator *left_ = nullptr;
  Operator *right_ = nullptr;
  bool round_done_ = true;
  JoinUnit *join_unit_ = nullptr;

  std::vector<Tuple*> left_table_tuples;
  std::vector<Tuple*> right_table_tuples;
  int total_num_;
  int current_index_;
  
  std::vector<CompositeTuple*> composite_table_tuples_;
  Tuple *tuple_;
};
