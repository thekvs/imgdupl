#!/usr/bin/env python

import sys
import struct
import random
import threading
import web
import sqlite3
import magic


class Clusters:

    def __init__(self):
        db_name = sys.argv[2]
        clusters_table_name = sys.argv[3]
        self._dbname = db_name
        self._clusters_table_name = clusters_table_name
        self._local = threading.local()
        cursor = self._cursor()
        cursor.execute('SELECT COUNT(*) FROM %s' % (self._clusters_table_name,))
        self._len = cursor.fetchone()[0]

    def __getitem__(self, cluster):
        cursor = self._cursor()
        qs = 'SELECT images FROM %s WHERE cluster_id=?' % (self._clusters_table_name,)
        cursor.execute(qs, (cluster,))
        images = cursor.fetchone()[0]
        return tuple(map(int, images.split(',')))

    def __contains__(self, cluster):
        cursor = self._cursor()
        qs = 'SELECT cluster_id FROM %s WHERE cluster_id=?' % (self._clusters_table_name,)
        cursor.execute(qs, (cluster,))
        return cursor.fetchone() is not None

    def random(self):
        cursor = self._cursor()
        qs = 'SELECT cluster_id FROM %s WHERE count>=2 ORDER BY RANDOM() LIMIT 1' % (self._clusters_table_name,)
        cursor.execute(qs)
        return cursor.fetchone()[0]

    def _cursor(self):
        conn = getattr(self._local, 'connection', None)
        if conn is None:
            conn = sqlite3.connect(self._dbname)
            self._local.connection = conn
        return conn.cursor()

    def _mime_detector(self):
        mime_detector = getattr(self._local, 'mime_detector', None)
        if mime_detector is None:
            mime_detector = magic.open(magic.MAGIC_MIME_TYPE)
            mime_detector.load()
            self._local.mime_detector = mime_detector
        return self._local.mime_detector
    
    def single_image(self, image_id):
        cursor = self._cursor()
        cursor.execute("SELECT path FROM hashes WHERE id=?", (image_id,))
        path = cursor.fetchone()[0]
        mime_type = self._mime_detector().file(path)
        return (path, mime_type)


clusters = Clusters()


urls = (
    '/', 'Index',
    '/(\d+)/grid', 'Thumbnails',
    '/(\d+)/list', 'Fullsize',
    '/(\d+)/image', 'Image'
    )


render = web.template.render('templates/')

form = web.form
clusterForm = form.Form(
    form.Textbox('show_cluster',
        form.notnull,
        form.regexp('\d+', 'Must be a digit'),
        form.Validator('Cluster not found', lambda x: int(x) in clusters),
        description='Show cluster:'))


def testForm():
    form = clusterForm()
    if web.input().get('show') and form.validates():
        web.seeother('/%s/list' % form.d['show_cluster'])
    return form


def render_base(form, contents):
    next_random = '/%s/grid' % clusters.random()
    return render.base(form, next_random, contents)


class Index:
    def GET(self):
        form = testForm()
        return render_base(form, '')


class Thumbnails:
    def GET(self, cluster):
        cluster = int(cluster)
        if cluster not in clusters:
            raise web.notfound()
        form = testForm()
        def make_url(img):
            return "/%i/image" % img
        return render_base(form, render.grid(cluster, clusters[cluster], make_url))


class Fullsize:
    def GET(self, cluster):
        cluster = int(cluster)
        if cluster not in clusters:
            raise web.notfound()
        form = testForm()
        def make_url(img):
            return "/%i/image" % img
        return render_base(form, render.list(cluster, clusters[cluster], make_url))


class Image:
    def GET(self, image_id):
        image_id = int(image_id)
        path, mime_type = clusters.single_image(image_id)
        data = open(path, 'rb').read()
        web.header("Content-Type", mime_type)
        web.header("Content-Length", len(data))
        return data


if __name__ == "__main__":
    app = web.application(urls, globals())
    app.run()
