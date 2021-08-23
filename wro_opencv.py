import math
import cv2
import numpy as np
import json
import urllib.request
from urllib.error import URLError
from pyzbar.pyzbar import decode
from time import sleep



# ===============================================================================
#   __      ____________ ________            __________.____________  ____ ___  #
#  /  \    /  \______   \\_____  \           \______   \   \_   ___ \|    |   \ #
#  \   \/\/   /|       _/ /   |   \   ______  |    |  _/   /    \  \/|    |   / #
#   \        / |    |   \/    |    \ /_____/  |    |   \   \     \___|    |  /  #
#    \__/\  /  |____|_  /\_______  /          |______  /___|\______  /______/   #
#         \/          \/         \/                  \/            \/           #
# ===============================================================================

# PROGRAMA QUE PROCESA IMAGENES SEGÚN EL MODO DE LA ESP32 Y DEVUELVE
# DATOS COMO RESULTADO DEL PROCESAMIENTO DE LAS IMAGENES.

url_img = "http://192.168.4.1/imagen/"
url_modo = "http://192.168.4.1/modo/"

# Remover estas dos líneas o comentarlas antes de la competencia.
winName = "LECTOR QR"
cv2.namedWindow(winName, cv2.WINDOW_AUTOSIZE)

#cap = cv2.VideoCapture(0)
#cap.set(3,800)
#cap.set(4,600)

while True:

	modoResponse = urllib.request.urlopen(url_modo)
	dModo = modoResponse.read()
	encoding = modoResponse.info().get_content_charset('utf-8')
	datoModo = json.loads(data.decode(encoding))
	print(datoModo)


	try:
		imgResponse = urllib.request.urlopen(url_img)
		imgNp = np.array(bytearray(imgResponse.read()),dtype=np.uint8)
		img = cv2.imdecode(imgNp,-1)

		img = cv2.rotate(img,cv2.ROTATE_90_CLOCKWISE)

		#success, img = cap.read()
		full_code = ""

		# Obtiene la cantidad de códigos QR detectados
		num_codigos = len(decode(img))

		# Si se detectan los 4 códigos, se envían a la ESP32
		if num_codigos == 4:

			for qrcode in decode(img):
				codigos = qrcode.data.decode('utf-8')
				print(codigos)
				full_code += codigos
				url2 = "http://192.168.4.1/codigo-qr/{0}".format(full_code)
				#url2 = "http://192.168.4.1/codigo-qr/{0}".format(codigos)

				codResponse = urllib.request.urlopen(url2)

				pts = np.array([qrcode.polygon],np.int32)
				pts = pts.reshape((-1,1,2))
				cv2.polylines(img,[pts],True,(255,0,255),3)
				pts2 = qrcode.rect 
				cv2.putText(img,qrcode.type,(pts2[0],pts2[1]),cv2.FONT_HERSHEY_SIMPLEX,0.9,(255,0,255),2)

	except URLError as e:
		print("Error en la URL: Servidor no encontrado")
		break 
	
	# Remover o comentar esta línea antes de la competencia.
	cv2.imshow(winName,img)
	sleep(0.02)

	# Esperamos a que se presione ESC para salir de la ejecución
	tecla = cv2.waitKey(5) & 0xFF
	if tecla == 27:
		break

# Remover o comentar esta línea antes de la competencia.
cv2.destroyAllWindows()








