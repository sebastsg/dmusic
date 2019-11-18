create or replace function get_album_release (
    in in_album_release_id int
)
returns table (
    "album_id"          int,
    "name"              varchar,
    "type"              varchar,
    "cover"             int,
    "album_release_id"  int,
    "release_type"      varchar,
    "catalog"           varchar,
    "released_at"       int,
    "group_id"          int
)
language plpgsql
as $$
begin
       return query
       select "album"."id",
              "album"."name",
              "album"."album_type_code",
              "album_attachment"."num",
              "album_release"."id",
              "album_release"."album_release_type_code",
              "album_release"."catalog",
              cast(extract(epoch from "album_release"."released_at") as int),
              "album_release_group"."group_id"
         from "album_release"
         join "album_release_group"
           on "album_release_group"."album_release_id" = "album_release"."id"
         join "album"
           on "album"."id" = "album_release"."album_id"
    left join "album_attachment"
           on "album_attachment"."album_release_id" = "album_release"."id"
          and "album_attachment"."type" = 'cover'
        where "album_release"."id" = in_album_release_id
     order by "album_release_group"."priority" asc;
end;
$$;
