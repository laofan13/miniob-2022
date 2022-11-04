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
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include <vector>
#include <unordered_map>
#include "rc.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"
#include "sql/expr/expression.h"

class Db;
class Table;
class FieldMeta;


class JoinUnit
{
public:
  JoinUnit() = default;
  ~JoinUnit(){
    delete filter_stmt_;
  }

  FilterStmt * filter_stmt() const {
    return filter_stmt_; 
  };

  void set_filter_stmt(FilterStmt * filter_stmt) {
    filter_stmt_ = filter_stmt;
  }

private:
  FilterStmt *filter_stmt_ = nullptr;
};

class JoinStmt 
{
public:

  JoinStmt() = default;
  virtual ~JoinStmt();

public:
  const std::vector<JoinUnit *> &join_units() const
  {
    return join_units_;
  }

public:
  static RC create(Db *db, std::vector<Table *> &tables, std::unordered_map<std::string, Table *> &table_map, 
			const JoinCond *join_conditions, int join_num,
			JoinStmt *&stmt);

  static RC create_join_unit(Db *db, std::vector<Table *> &tables, std::unordered_map<std::string, Table *> &table_map, 
			       const JoinCond &join_condition, JoinUnit *&join_unit);

private:
  std::vector<JoinUnit *>  join_units_; // 默认当前都是AND关系
};