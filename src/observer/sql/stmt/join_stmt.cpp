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

#include "rc.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/stmt/join_stmt.h"
#include "storage/common/db.h"
#include "storage/common/table.h"

#include "sql/stmt/filter_stmt.h"

JoinStmt::~JoinStmt()
{
  for (JoinUnit *unit : join_units_) {
    delete unit;
  }
  join_units_.clear();
}

RC JoinStmt::create(Db *db, std::vector<Table *> &tables, std::unordered_map<std::string, Table *> &table_map, 
			const JoinCond *join_conditions, int join_num,
			JoinStmt *&stmt)
{
  RC rc = RC::SUCCESS;
  stmt = nullptr;

  JoinStmt *join_stmt = new JoinStmt();
  for (int i = 0; i < join_num; i++) {
    JoinUnit *join_unit = nullptr;
    rc = create_join_unit(db, tables, table_map, join_conditions[i], join_unit);
    if (rc != RC::SUCCESS) {
      delete join_unit;
      LOG_WARN("failed to create filter unit. condition index=%d", i);
      return rc;
    }
    join_stmt->join_units_.push_back(join_unit);
  }

  stmt = join_stmt;
  return rc;
}

RC JoinStmt::create_join_unit(Db *db, std::vector<Table *> &tables, std::unordered_map<std::string, Table *> &table_map, 
			       const JoinCond &join_condition, JoinUnit *&join_unit)
{
  RC rc = RC::SUCCESS;

  // create filter statement in `where` statement
  FilterStmt *filter_stmt = nullptr;
  rc = FilterStmt::create(db, tables[0], &table_map,
            join_condition.conditions, join_condition.condition_num, filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot construct filter stmt");
    return rc;
  }
  
  join_unit = new JoinUnit;
  join_unit->set_filter_stmt(filter_stmt);

  // 检查两个类型是否能够比较
  return rc;
}
