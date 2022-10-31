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

#include "rc.h"
#include "sql/stmt/stmt.h"
#include "storage/common/field.h"
#include "sql/stmt/filter_stmt.h"

class Table;

class UpdateStmt : public Stmt
{
public:
  UpdateStmt() = default;
  UpdateStmt(Table *table, std::vector<UpdateField> &update_fields_, FilterStmt *filter_stmt, int value_amount);
  ~UpdateStmt() override;

  StmtType type() const override { return StmtType::UPDATE;}
public:
  static RC create(Db *db, const Updates &update_sql, Stmt *&stmt);

public:
  Table *table() const {return table_;}
  const std::vector<UpdateField> &update_fields() const { return update_fields_; }
  FilterStmt *filter_stmt() const { return filter_stmt_; }
  int value_amount() const { return value_amount_; }

private:
  Table *table_ = nullptr;
  std::vector<UpdateField> update_fields_;
  FilterStmt *filter_stmt_ = nullptr;
  int value_amount_ = 0;
};

