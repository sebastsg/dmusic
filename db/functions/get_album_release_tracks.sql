create or replace function get_album_release_tracks (
    in in_album_release_id int
)
returns table (
    "disc_num"         int,
    "num"              int,
    "seconds"          int,
    "name"             varchar
)
language plpgsql
as $$
begin
    return query
    select "track"."disc_num",
           "track"."num",
           "track"."seconds",
           "track"."name"
      from "track"
     where "track"."album_release_id" = in_album_release_id
  order by "track"."album_release_id" asc,
           "track"."disc_num" asc,
           "track"."num" asc;
end;
$$;
