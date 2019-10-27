create or replace function search_tag (
    in in_name  varchar(160),
    in in_limit int
)
returns table (
    "code" varchar,
    "name" varchar
)
language plpgsql
as $$
begin
    return query
    select "tag"."name",
           "tag"."name"
      from "tag"
     where lower("tag"."name") like lower(in_name)
     limit in_limit;
end;
$$;
