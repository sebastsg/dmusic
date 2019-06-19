create or replace function get_group_tags (
    in in_group_id int
)
returns table (
    "tag" varchar
)
language plpgsql
as $$
begin
      return query
      select "tag_name"
        from "group_tag"
       where "group_id" = in_group_id
    order by "priority";
end;
$$;
