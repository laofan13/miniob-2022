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
// Created by WangYunlai on 2022/6/9.
//

#pragma once

#include "sql/operator/operator.h"
#include "rc.h"

class Trx;
class DeleteStmt;

class SortOperator : public Operator
{
public:
  SortOperator(const std::vector<SortField> &sort_fields):sort_fields_(sort_fields)
  {
    current_index_ = 0;
  }

  virtual ~SortOperator() {
    if(children_[0]) {
      delete children_[0];
      children_[0] = nullptr;
    }
  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override;
  //int tuple_cell_num() const override
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override

private:
  const std::vector<SortField> &sort_fields_;
  std::vector<Tuple*> table_tuples_;
  int current_index_;
  Tuple *tuple_;
};
