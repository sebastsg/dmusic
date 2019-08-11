create table "country" (
    "code" varchar(8)   not null primary key,
    "name" varchar(128) not null unique
);

create table "language" (
    "code" varchar(8)   not null primary key,
    "name" varchar(128) not null unique
);

create table "role" (
    "code" varchar(64)  not null primary key,
    "name" varchar(128) not null unique
);

create table "album_type" (
    "code"   varchar(16) not null primary key,
    "name"   varchar(64) not null unique,
    "plural" varchar(64) not null unique
);

create table "album_release_type" (
    "code" varchar(32) not null primary key,
    "name" varchar(64) not null unique
);

create table "audio_format" (
    "code"     varchar(16) not null primary key,
    "name"     varchar(16) not null unique,
    "quality"  int         not null,
    "lossless" int         not null
);

create table "tag" (
    "name" varchar(64) not null primary key
);

create table "person" (
    "id"            serial       not null primary key,
    "first_name"    varchar(128) not null,
    "last_name"     varchar(128) not null,
    "country_code"  varchar(8)   not null,
    "born_at"       date         not null,
    "dead_at"       date         not null,

    constraint  fk_person_country
    foreign key ("country_code") references "country" ("code")
);

create table "group" (
    "id"           serial       not null primary key,
    "country_code" varchar(8)   not null,
    "name"         varchar(160) not null,
    "website"      varchar(160) not null,
    "description"  text         not null,

    constraint  fk_group_country
    foreign key ("country_code") references "country" ("code")
);

create table "group_relation" (
    "group_1_id" int not null,
    "group_2_id" int not null,
    
    constraint  fk_group_relation_group_1
    foreign key ("group_1_id") references "group" ("id"),
    
    constraint  fk_group_relation_group_2
    foreign key ("group_2_id") references "group" ("id"),

    constraint  pk_group_relation
    primary key ("group_1_id", "group_2_id")
);

create table "group_alias" (
    "group_id" int          not null,
    "priority" int          not null,
    "name"     varchar(160) not null,
    
    constraint  fk_group_alias_group
    foreign key ("group_id") references "group" ("id"),

    constraint  pk_group_alias
    primary key ("group_id", "name")
);

create table "group_tag" (
    "group_id" int         not null,
    "tag_name" varchar(64) not null,
    "priority" int         not null,

    constraint  fk_group_tag_group
    foreign key ("group_id") references "group" ("id"),

    constraint  fk_group_tag_tag
    foreign key ("tag_name") references "tag" ("name"),

    constraint  pk_group_tag
    primary key ("group_id", "tag_name")
);

create table "group_image" (
    "group_id"    int  not null,
    "num"         int  not null,
    "description" text not null,

    constraint  fk_group_image_group
    foreign key ("group_id") references "group" ("id"),
    
    constraint  pk_album_image
    primary key ("group_id", "num")
);

create table "group_activity" (
    "group_id"   int  not null,
    "started_at" date not null,
    "stopped_at" date not null,
    
    constraint  fk_group_activity_group
    foreign key ("group_id") references "group" ("id"),

    constraint  pk_group_activity
    primary key ("group_id", "started_at")
);

create table "group_member" (
    "group_id"    int         not null,
    "person_id"   int         not null,
    "role_code"   varchar(32) not null,
    "started_at"  date        not null,
    "stopped_at"  date        not null,
    
    constraint  fk_group_member_group
    foreign key ("group_id") references "group" ("id"),

    constraint  fk_group_member_person
    foreign key ("person_id") references "person" ("id"),

    constraint  fk_group_member_role
    foreign key ("role_code") references "role" ("code"),

    constraint  pk_group_member
    primary key ("group_id", "person_id", "role_code", "started_at", "stopped_at")
);

create table "album" (
    "id"               serial       not null primary key,
    "name"             varchar(128) not null,
    "album_type_code"  varchar(16)  not null,

    constraint  fk_album_album_type_code
    foreign key ("album_type_code") references "album_type" ("code")
);

create table "album_tag" (
    "album_id" int         not null,
    "tag_name" varchar(64) not null,
    "priority" int         not null,

    constraint  fk_album_tag_album
    foreign key ("album_id") references "album" ("id"),

    constraint  fk_album_tag_tag
    foreign key ("tag_name") references "tag" ("name"),

    constraint  pk_album_tag
    primary key ("album_id", "tag_name")
);

create table "album_release" (
    "id"                       serial       not null primary key,
    "album_id"                 int          not null,
    "album_release_type_code"  varchar(32)  not null,
    "catalog"                  varchar(160) not null,
    "released_at"              date         not null,

    constraint  fk_album_release_album
    foreign key ("album_id") references "album" ("id"),

    constraint  fk_album_release_album_release_type
    foreign key ("album_release_type_code") references "album_release_type" ("code")
);

create table "album_attachment" (
    "album_release_id"  int         not null,
    "num"               int         not null,
    "type"              varchar(16) not null,
    "description"       text        not null,

    constraint  fk_album_release_image_album
    foreign key ("album_release_id") references "album_release" ("id"),
    
    constraint  pk_album_attachment
    primary key ("album_release_id", "num")
);

create table "album_release_group" (
    "album_release_id" int not null,
    "group_id"         int not null,
    "priority"         int not null,
    
    constraint  fk_album_release_group_album_release
    foreign key ("album_release_id") references "album_release" ("id"),

    constraint  fk_album_release_group_group
    foreign key ("group_id") references "group" ("id"),

    constraint  pk_album_release_group
    primary key ("album_release_id", "group_id")
);

create table "disc" (
    "album_release_id" int          not null,
    "num"              int          not null,
    "name"             varchar(128) not null,

    constraint  fk_disc_album_release
    foreign key ("album_release_id") references "album_release" ("id"),

    constraint  pk_disc
    primary key ("album_release_id", "num")
);

create table "track" (
    "album_release_id" int          not null,
    "disc_num"         int          not null,
    "num"              int          not null,
    "seconds"          int          not null,
    "name"             varchar(256) not null,
    
    constraint  fk_track_disc
    foreign key ("album_release_id", "disc_num") references "disc" ("album_release_id", "num"),

    constraint  pk_track
    primary key ("album_release_id", "disc_num", "num")
);

create table "lyrics" (
    "album_release_id"  int        not null,
    "disc_num"          int        not null,
    "track_num"         int        not null,
    "language_code"     varchar(8) not null,
    "text"              text       not null,
    
    constraint  fk_lyrics_track
    foreign key ("album_release_id", "disc_num", "track_num") references "track" ("album_release_id", "disc_num", "num"),

    constraint  fk_lyrics_language
    foreign key ("language_code") references "language" ("code"),

    constraint  pk_lyrics
    primary key ("album_release_id", "disc_num", "track_num", "language_code")
);

create table "track_hash" (
    "album_release_id" int       not null,
    "disc_num"         int       not null,
    "track_num"        int       not null,
    "file_hash"        text      not null,
    "hashed_at"        timestamp not null,
    
    constraint  fk_track_hash_track
    foreign key ("album_release_id", "disc_num", "track_num") references "track" ("album_release_id", "disc_num", "num"),

    constraint  pk_track_hash
    primary key ("album_release_id", "disc_num", "track_num", "hashed_at")
);

create table "user" (
    "name"          varchar(32) not null primary key,
    "password_hash" varchar(96) not null,
    "salt"          varchar(96) not null,
    "stream_method" varchar(32) not null default 'stream',
    "is_admin"      int         not null default 0,
    "created_at"    timestamp   not null default current_timestamp
);

create table "playlist" (
    "id"        serial      not null primary key,
    "user_name" varchar(32) not null,
    "name"      varchar(64) not null,

    constraint  fk_playlist_user
    foreign key ("user_name") references "user" ("name")
);

create table "playlist_track" (
    "playlist_id"      int not null,
    "album_release_id" int not null,
    "disc_num"         int not null,
    "track_num"        int not null,
    "num"              int not null,

    constraint  fk_playlist_track_playlist
    foreign key ("playlist_id") references "playlist" ("id"),

    constraint  fk_playlist_track_track
    foreign key ("album_release_id", "disc_num", "track_num") references "track" ("album_release_id", "disc_num", "num"),

    constraint  pk_playlist_track
    primary key ("playlist_id", "num")
);

create table "session_track" (
    "user_name"        varchar(32) not null,
    "album_release_id" int         not null,
    "disc_num"         int         not null,
    "track_num"        int         not null,
    "num"              int         not null,

    constraint  fk_session_track_user
    foreign key ("user_name") references "user" ("name"),

    constraint  fk_session_track_track
    foreign key ("album_release_id", "disc_num", "track_num") references "track" ("album_release_id", "disc_num", "num"),

    constraint  pk_session_track
    primary key ("user_name", "num")
);

create table "favourite_group" (
    "user_name" varchar(32) not null,
    "group_id"  int         not null,
    "num"       int         not null,

    constraint  fk_favourite_group_user
    foreign key ("user_name") references "user" ("name"),

    constraint  fk_favourite_group_group
    foreign key ("group_id") references "group" ("id"),

    constraint  pk_favourite_group
    primary key ("user_name", "group_id", "num")
);

create table "music_video" (
    "id"          serial       not null primary key,
    "group_id"    int          not null,
    "name"        varchar(256) not null,
    "description" text         not null,
    "released_at" date         not null,

    constraint  fk_music_video_group
    foreign key ("group_id") references "group" ("id")
);

create table "album_release_format" (
    "album_release_id"  int         not null,
    "audio_format_code" varchar(16) not null,

    constraint  fk_album_release_format_album_release
    foreign key ("album_release_id") references "album_release" ("id"),

    constraint  fk_album_release_format_audio_format
    foreign key ("audio_format_code") references "audio_format" ("code"),

    constraint  pk_album_release_format
    primary key ("album_release_id", "audio_format_code")
);
