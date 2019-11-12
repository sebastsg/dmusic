create or replace function get_favourite_groups (
    in in_user_name varchar(32)
)
returns table (
    "id"    int,
    "name"  varchar
)
language plpgsql
as $$
begin
    return query
    select "group"."id",
           "group"."name"
      from "favourite_group"
      join "group"
        on "group"."id" = "favourite_group"."group_id"
     where "favourite_group"."user_name" = in_user_name
  order by "favourite_group"."num" asc;
end;
$$;
