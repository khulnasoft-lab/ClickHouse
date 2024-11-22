#pragma once
#include <Processors/QueryPlan/ITransformingStep.h>
#include <Interpreters/ActionsDAG.h>
#include <Interpreters/Cache/QueryConditionCache.h>

namespace DB
{

/// Implements WHERE, HAVING operations. See FilterTransform.
class FilterStep : public ITransformingStep
{
public:
    FilterStep(
        const Header & input_header_,
        ActionsDAG actions_dag_,
        String filter_column_name_,
        bool remove_filter_column_);

    String getName() const override { return "Filter"; }
    void transformPipeline(QueryPipelineBuilder & pipeline, const BuildQueryPipelineSettings & settings) override;

    void describeActions(JSONBuilder::JSONMap & map) const override;
    void describeActions(FormatSettings & settings) const override;

    const ActionsDAG & getExpression() const { return actions_dag; }
    ActionsDAG & getExpression() { return actions_dag; }
    const String & getFilterColumnName() const { return filter_column_name; }
    bool removesFilterColumn() const { return remove_filter_column; }
    void setQueryConditionCacheAndKey(QueryConditionCachePtr query_condition_cache_, const String & where_condition_);

private:
    void updateOutputHeader() override;

    ActionsDAG actions_dag;
    String filter_column_name;
    bool remove_filter_column;

    QueryConditionCachePtr query_condition_cache;
    std::optional<String> where_condition;
};

}
