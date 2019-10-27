create or replace function get_group_tags (
    in in_group_id int
)
returns table (
    "tag"      varchar,
    "priority" int
)
language plpgsql
as $$
begin
      return query
      select "group_tag"."tag_name",
             "group_tag"."priority"
        from "group_tag"
       where "group_tag"."group_id" = in_group_id
    order by "group_tag"."priority";
end;
$$;
