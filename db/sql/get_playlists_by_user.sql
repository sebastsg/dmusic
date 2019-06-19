create or replace function get_playlists_by_user (
    in in_user_name varchar(32)
)
returns table (
    "id"   int,
    "name" varchar
)
language plpgsql
as $$
begin
    return query
    select "playlist"."id",
           "playlist"."name"
      from "playlist"
     where "playlist"."user_name" = in_user_name;
end;
$$;
