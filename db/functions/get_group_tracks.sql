create or replace function get_group_tracks (
    in in_group_id int
)
returns table (
    "album_release_id" int,
    "disc_num"         int,
    "num"              int,
    "seconds"          int,
    "name"             varchar
)
language plpgsql
as $$
begin
    return query
    select "track"."album_release_id",
           "track"."disc_num",
           "track"."num",
           "track"."seconds",
           "track"."name"
      from "group"
      join "album_release_group"
        on "album_release_group"."group_id" = "group"."id"
      join "track"
        on "track"."album_release_id" = "album_release_group"."album_release_id"
     where "group"."id" = in_group_id
  order by "track"."album_release_id" asc,
           "track"."disc_num" asc,
           "track"."num" asc;
end;
$$;
