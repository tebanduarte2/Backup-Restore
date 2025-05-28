import os
import io
from flask import Flask, request, jsonify, send_file
import boto3
from botocore.exceptions import NoCredentialsError, ClientError

# No es estrictamente necesario si las credenciales están hardcodeadas,
# pero es una buena práctica si en el futuro decides usar .env
# from dotenv import load_dotenv
# load_dotenv()

app = Flask(__name__)

# Configuración del cliente de S3
# ATENCIÓN: Hardcodear credenciales en el código no es seguro para producción.
# Para este ejemplo, se mantiene como lo solicitaste.
s3 = boto3.client(
    's3',
    aws_access_key_id="AKIA356SJ4SHYZVPLROU",
    aws_secret_access_key="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",
    region_name="us-east-1"
)
S3_BUCKET = "sistemas-operativos-bucket" # 


@app.route('/upload-backup', methods=['POST'])
def upload_backup():
    """
    Endpoint para recibir un archivo ZIP de una aplicación C++ y subirlo a S3.
    Espera un archivo bajo el campo 'backup_file' en la petición multipart/form-data.
    """
    if 'backup_file' not in request.files:
        return jsonify({"success": False, "message": "No se encontró el archivo 'backup_file' en la petición."}), 400

    uploaded_file = request.files['backup_file']

    if uploaded_file.filename == '':
        return jsonify({"success": False, "message": "Nombre de archivo vacío."}), 400

    if not uploaded_file.filename.lower().endswith('.zip'):
        return jsonify({"success": False, "message": "Tipo de archivo no permitido. Solo se aceptan archivos ZIP."}), 400

    s3_file_name = uploaded_file.filename

    try:
        # Leer el contenido del archivo en un objeto de bytes en memoria
        file_content = uploaded_file.read()
        file_object = io.BytesIO(file_content)

        s3.upload_fileobj(
            file_object,        # El objeto de archivo en memoria
            S3_BUCKET,          # Tu bucket S3
            s3_file_name,       # El nombre que tendrá el archivo en S3
            ExtraArgs={'ContentType': 'application/zip'} # Asegura el tipo MIME correcto
        )

        s3_url = f"https://{S3_BUCKET}.s3.{s3.meta.region_name}.amazonaws.com/{s3_file_name}"
        print(f"Archivo subido exitosamente a S3: {s3_url}")

        return jsonify({
            "success": True,
            "message": "Archivo ZIP subido exitosamente a S3.",
            "s3_url": s3_url,
            "filename_on_s3": s3_file_name
        }), 200

    except NoCredentialsError:
        print("Error: Las credenciales de AWS no están configuradas o son inválidas.")
        return jsonify({"success": False, "message": "Error de credenciales de AWS. Verifica tu configuración."}), 500
    except ClientError as e:
        error_code = e.response.get("Error", {}).get("Code")
        error_message = e.response.get("Error", {}).get("Message")
        print(f"Error de cliente de S3 ({error_code}): {error_message}")
        return jsonify({"success": False, "message": f"Error al subir a S3: {error_message} (Código: {error_code})"}), 500
    except Exception as e:
        print(f"Ocurrió un error inesperado durante la subida a S3: {e}")
        return jsonify({"success": False, "message": f"Error interno del servidor: {str(e)}"}), 500

@app.route('/list-backups', methods=['GET'])
def list_backups():
    """
    Endpoint para listar los archivos (objetos) disponibles en el bucket S3.
    Devuelve una lista de nombres de archivos.
    """
    try:
        response = s3.list_objects_v2(Bucket=S3_BUCKET)
        files = []
        if 'Contents' in response:
            for obj in response['Contents']:
                files.append(obj['Key']) # 'Key' es el nombre del archivo en S3
        
        print(f"Archivos listados en S3: {files}")
        return jsonify({"success": True, "files": files}), 200

    except NoCredentialsError:
        print("Error: Las credenciales de AWS no están configuradas o son inválidas.")
        return jsonify({"success": False, "message": "Error de credenciales de AWS. Verifica tu configuración."}), 500
    except ClientError as e:
        error_code = e.response.get("Error", {}).get("Code")
        error_message = e.response.get("Error", {}).get("Message")
        print(f"Error de cliente de S3 ({error_code}): {error_message}")
        return jsonify({"success": False, "message": f"Error al listar archivos de S3: {error_message} (Código: {error_code})"}), 500
    except Exception as e:
        print(f"Ocurrió un error inesperado al listar archivos de S3: {e}")
        return jsonify({"success": False, "message": f"Error interno del servidor: {str(e)}"}), 500

@app.route('/download-backup/<path:filename>', methods=['GET'])
def download_backup(filename):
    """
    Endpoint para descargar un archivo específico del bucket S3.
    El <path:filename> permite que el nombre del archivo incluya barras (subdirectorios).
    """
    try:
        # Descargar el archivo de S3 a un objeto de bytes en memoria
        file_object = io.BytesIO()
        s3.download_fileobj(S3_BUCKET, filename, file_object)
        file_object.seek(0) # Mover el puntero al inicio del archivo para la lectura

        print(f"Archivo '{filename}' descargado de S3.")
        # Servir el archivo al cliente C++
        return send_file(
            file_object,
            mimetype='application/zip', # O el mimetype real del archivo si es variable
            as_attachment=True,
            download_name=filename # Nombre del archivo cuando se descarga
        )

    except NoCredentialsError:
        print("Error: Las credenciales de AWS no están configuradas o son inválidas.")
        return jsonify({"success": False, "message": "Error de credenciales de AWS. Verifica tu configuración."}), 500
    except ClientError as e:
        error_code = e.response.get("Error", {}).get("Code")
        error_message = e.response.get("Error", {}).get("Message")
        print(f"Error de cliente de S3 ({error_code}): {error_message}")
        if error_code == 'NoSuchKey':
            return jsonify({"success": False, "message": f"Archivo '{filename}' no encontrado en S3."}), 404
        return jsonify({"success": False, "message": f"Error al descargar de S3: {error_message} (Código: {error_code})"}), 500
    except Exception as e:
        print(f"Ocurrió un error inesperado durante la descarga de S3: {e}")
        return jsonify({"success": False, "message": f"Error interno del servidor: {str(e)}"}), 500

# Tu código existente de Flask para otras rutas o funcionalidades
# ...

if __name__ == '__main__':
    # Para que la app sea accesible desde otras máquinas en tu red (si C++ no está en la misma)
    # usa host='0.0.0.0'. Si solo es local, '127.0.0.1' es suficiente.
    # debug=True es útil para desarrollo, pero desactívalo en producción.
    app.run(debug=True, host='0.0.0.0', port=5000)
