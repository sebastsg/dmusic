create or replace function search_groups (
    in in_name  varchar(160),
    in in_limit int
)
returns table (
    "id"   int,
    "name" varchar
)
language plpgsql
as $$
begin
    return query
    select distinct
	       "group"."id",
           "group"."name"
      from "group"
	  join "group_alias"
	    on "group_alias"."group_id" = "group"."id"
     where lower("group"."name") like lower(in_name)
	    or lower("group_alias"."name") like lower(in_name)
     limit in_limit;
end;
$$;
