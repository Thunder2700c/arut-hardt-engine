package com.arut.engine;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.MediaStore;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class LogExporter {
    private LogExporter() {}

    public static boolean exportRuntimeLog(Activity activity, String sourcePath, String previousPath) {
        if (activity == null || sourcePath == null) return false;
        File source = new File(sourcePath);
        File previous = previousPath == null ? null : new File(previousPath);
        if (!source.isFile()) return false;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            return exportWithMediaStore(activity, source, previous);
        }
        return exportToAppDocuments(activity, source, previous);
    }

    private static boolean exportWithMediaStore(Activity activity, File source, File previous) {
        ContentResolver resolver = activity.getContentResolver();
        String relativePath = Environment.DIRECTORY_DOWNLOADS + "/ARUT-HARDT/";
        Uri collection = MediaStore.Downloads.EXTERNAL_CONTENT_URI;
        String selection = MediaStore.MediaColumns.DISPLAY_NAME + "=? AND " +
                MediaStore.MediaColumns.RELATIVE_PATH + "=?";
        String[] selectionArgs = {"ARUT-HARDT-runtime.log", relativePath};

        try (Cursor cursor = resolver.query(collection,
                new String[]{MediaStore.MediaColumns._ID}, selection, selectionArgs, null)) {
            if (cursor != null) {
                while (cursor.moveToNext()) {
                    long id = cursor.getLong(0);
                    resolver.delete(Uri.withAppendedPath(collection, Long.toString(id)), null, null);
                }
            }
        } catch (RuntimeException ignored) {
            // Continue with a fresh insert; a stale previous export is harmless.
        }

        ContentValues values = new ContentValues();
        values.put(MediaStore.MediaColumns.DISPLAY_NAME, "ARUT-HARDT-runtime.log");
        values.put(MediaStore.MediaColumns.MIME_TYPE, "text/plain");
        values.put(MediaStore.MediaColumns.RELATIVE_PATH, relativePath);
        Uri destination = resolver.insert(collection, values);
        if (destination == null) return false;

        try (InputStream input = new FileInputStream(source);
             OutputStream output = resolver.openOutputStream(destination, "w")) {
            if (output == null) {
                resolver.delete(destination, null, null);
                return false;
            }
            copyLogs(previous, input, output);
            return true;
        } catch (IOException | RuntimeException error) {
            resolver.delete(destination, null, null);
            return false;
        }
    }

    private static boolean exportToAppDocuments(Activity activity, File source, File previous) {
        File documents = activity.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS);
        if (documents == null) return false;
        File destination = new File(documents, "ARUT-HARDT-runtime.log");
        try (InputStream input = new FileInputStream(source);
             OutputStream output = new FileOutputStream(destination, false)) {
            copyLogs(previous, input, output);
            return true;
        } catch (IOException error) {
            return false;
        }
    }

    private static void copyLogs(File previous, InputStream current, OutputStream output) throws IOException {
        if (previous != null && previous.isFile()) {
            try (InputStream old = new FileInputStream(previous)) {
                copy(old, output);
                output.write("\\n--- current runtime.log ---\\n".getBytes(java.nio.charset.StandardCharsets.UTF_8));
            }
        }
        copy(current, output);
    }

    private static void copy(InputStream input, OutputStream output) throws IOException {
        byte[] buffer = new byte[8192];
        int count;
        while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
        output.flush();
    }
}
