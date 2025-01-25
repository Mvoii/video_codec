from flask import Flask, request, render_template, send_from_directory, redirect, url_for, jsonify
import os
import logging
from werkzeug.utils import secure_filename
from video_encoder import VideoEncoder
from functools import wraps
import magic
import json

# config
UPLOAD_FOLDER = "uploads"
COMPRESSED_FOLDER = "compressed"
DECOMPRESSED_FOLDER = "decompressed"
ALLOWED_EXTENSIONS = {"rgb24","bin"}
MAX_FILE_SIZE = 100 * 1024 * 1024 # 100MB

app = Flask(__name__)
app.config["UPLOAD_FOLDER"] = UPLOAD_FOLDER
app.config["MAX_CONTENT_LENGTH"] = MAX_FILE_SIZE

if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)
if not os.path.exists(COMPRESSED_FOLDER):
    os.makedirs(COMPRESSED_FOLDER)
if not os.path.exists(DECOMPRESSED_FOLDER):
    os.makedirs(DECOMPRESSED_FOLDER)


logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class VideoProcessingError(Exception):
    """Custom exceptions for video processing errors"""
    pass

# helpers
def error_response(message, status_code=400):
    return jsonify({
        "success": False,
        "error": message
    }), status_code

def success_response(data, message="Operation successful"):
    return jsonify({
        "success": True,
        "message": message,
        "data": data
    })

def validate_file_request(f):
    """decorator to validate fiel uploads"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if "file" not in request.files:
            return error_response('No file provided')
        
        file = request.files["file"]
        if file.filename == '':
            return error_response("No file selected")
        
        if not allowed_file(file.filename):
            return error_response(f"Invalid file type. Allowed types: {', '.join(ALLOWED_EXTENSIONS)}")
        
        try:
            width = int(request.form.get("width", 0))
            height = int(request.form.get('height', 0))
            if width <= 0 or height <= 0:
                return error_response("Invalid video dimensions")
        except ValueError:
            return error_response("Invalid video dimension format")
        
        return f(*args, **kwargs)
    return decorated_function

def allowed_file(filename):
    return ('.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS)

def validate_file_content(file_path, expected_type):
    """validate file content using magic numbers"""
    mime = magic.Magic(mime=True)
    file_type = mime.from_file(file_path)
    
    if expected_type == "rgb24":
        # raw vids may be dectected as var bin types
        return file_type.startswith("application/") or file_type == "video/raw"
    elif expected_type == "bin":
        return file_type.startswith('application/') or 'compressed' in file_type.lower()

    return False

@app.errorhandler(Exception)
def handle_exception(e):
    logging.error(f"unhandled exception; {str(e)}")
    return jsonify({"success": False, "error": str(e)}), 500

@app.route('/')
def index():
    return render_template("index.html")


@app.route("/api/compress", methods=["POST"])
@validate_file_request
def compress():
    try:
        file = request.files['file']
        width = int(request.form['width'])
        height = int(request.form['height'])
        
        # save uploaded file
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config["UPLOAD_FOLDER"], filename)
        file.save(filepath)
        
        # validate file content
        if not validate_file_content(filepath, 'rgb24'):
            os.remove(filepath)
            return error_response("Invalid file content")
        
        # init codec
        encoder = VideoEncoder(width, height)
        
        with open(filepath, 'rb') as f:
            input_data = f.read()

        frames = encoder.read_frames(input_data)
        if not frames:
            raise VideoProcessingError('No valid frames found in input')
        compressed_data = encoder.encode_frames(frames)

        # save compressed file
        compressed_filename = f"{os.path.splitext(filename)[0]}_compressed.bin"
        compressed_path = os.path.join(COMPRESSED_FOLDER, compressed_filename)

        with open(compressed_path, 'wb') as f:
            f.write(compressed_data)

        # calculate compression stats
        original_size = os.path.getsize(filepath)
        compressed_size = os.path.getsize(compressed_path)
        compression_ratio = (1 - (compressed_size / original_size)) * 100

        response_data = {
            'filename': compressed_filename,
            'original_size': original_size,
            "compressed_size": compressed_size,
            'compression_ratio': f"{compression_ratio:.2f}%",
            'frame_count': len(frames),
            'download_url': f'api/download/compressed/{compressed_filename}'
        }
        print("DEBUG compress response: ", response_data)
        
        return success_response(response_data)

    except VideoProcessingError as e:
        logger.error(f"Video processing error: {str(e)}")
        return error_response(str(e))
    except Exception as e:
        logger.error(f"Unexpected error during compression: {str(e)}")
        return error_response("An unexpected error occured suring compression")
    finally:
        # clean ups
        if os.path.exists(filepath):
            os.remove(filepath)
    

@app.route('/api/decompress', methods=['POST'])
@validate_file_request
def decompress():
    try:
        file = request.files['file']
        width = int(request.form["width"])
        height = int(request.form["height"])
        num_frames = int(request.form.get("num_frames", 0))
        
        if num_frames <= 0:
            return error_response("Invalid number of frames")

        # save uploaded file
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config["UPLOAD_FOLDER"], filename)
        file.save(filepath)
        
        # validate file content
        if not validate_file_content(filepath, "bin"):
            os.remove(filepath)
            return error_response("Invalid file content")

        # init codec
        encoder = VideoEncoder(width, height)
        
        with open(filepath, 'rb') as f:
            compressed_data = f.read()

        decompressed_frames = encoder.decode_frames(compressed_data, num_frames)
        if not decompressed_frames:
            raise VideoProcessingError("Failed to decompress frames")
        
        # save decompressed frames
        decompressed_filename = f"{os.path.splitext(filename)[0]}_decompressed.rgb24"
        decompressed_path = os.path.join(DECOMPRESSED_FOLDER, decompressed_filename)

        with open(decompressed_path, "wb") as f:
            for frame in decompressed_frames:
                f.write(frame.tobytes())
        
        return success_response({
            'filename': decompressed_filename,
            'frame_count': len(decompressed_frames),
            'size': os.path.getsize(decompressed_path),
            'download_url': f"/api/download/decompressed/{decompressed_filename}"
        })
    except VideoProcessingError as e:
        logger.error(f"Video processing error: {str(e)}")
        return error_response(str(e))
    finally:
        # clean ups
        if os.path.exists(filepath):
            os.remove(filepath)
        

@app.route('/api/download/<folder>/<filename>')
def download_file(folder, filename):
    try:
        if folder not in ['compressed', 'decompressed']:
            return error_response("Invalid download folder", 404)
        
        directory = COMPRESSED_FOLDER if folder == 'compressed' else DECOMPRESSED_FOLDER
        return send_from_directory(directory, filename, as_attachment=True)
    except Exception as e:
        logger.error(f"Download error: {str(e)}")
        return error_response("Fiel not found", 404)


@app.errorhandler(413)
def request_entity_too_large(error):
    """handle file too big error"""
    return error_response(f"File too large. Max size: {MAX_FILE_SIZE/(1024*1024)}MB", 413)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", 5000)), debug=True)
