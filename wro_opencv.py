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

#tl = [0.8,1.2]			# Margen del 20% en limites de colores
tl = [0.85,1.15]		# Margen del 15% en limites de colores
#tl = [0.9,1.1]			# Margen del 10% en limites de colores

lim = 200				# Reducir este limite levemente en caso de
						# no detectar los colores correctamente

#cap = cv2.VideoCapture(0)
#cap.set(3,800)
#cap.set(4,600)

while True:

	modoResponse = urllib.request.urlopen(url_modo)
	dModo = modoResponse.read()
	encoding = modoResponse.info().get_content_charset('utf-8')
	datoModo = json.loads(data.decode(encoding))
	print(datoModo)

	if datoModo['modo'] == 'qr':

		# Remover estas dos líneas o comentarlas antes de la competencia.
		winName = "LECTURA QR"
		cv2.namedWindow(winName, cv2.WINDOW_AUTOSIZE)

		try:
			imgResponse = urllib.request.urlopen(url_img)
			
		except URLError as e:
			print("Error en la URL: Servidor no encontrado " + e)
			break 

		imgNp = np.array(bytearray(imgResponse.read()),dtype=np.uint8)
		img = cv2.imdecode(imgNp,-1)

		img = cv2.rotate(img,cv2.ROTATE_90_CLOCKWISE)

		#success, img = cap.read()
		full_code = ""

		# Obtiene la cantidad de matrices QR detectados
		num_codigos = len(decode(img))

		# Si se detectan los 4 códigos, se envían a la ESP32
		if num_codigos == 4:

			for qrcode in decode(img):
				
				pts = np.array([qrcode.polygon],np.int32)
				pts = pts.reshape((-1,1,2))
				cv2.polylines(img,[pts],True,(255,0,255),2)
				pts2 = qrcode.rect 
				cv2.putText(img,qrcode.type,(pts2[0],pts2[1]),cv2.FONT_HERSHEY_SIMPLEX,0.9,(255,0,255),2)

				codigos = qrcode.data.decode('utf-8')
				print(codigos)
				full_code += codigos
				url2 = "http://192.168.4.1/codigo-qr/{0}".format(full_code)
				#url2 = "http://192.168.4.1/codigo-qr/{0}".format(codigos)

				codResponse = urllib.request.urlopen(url2)

				if codResponse.status == 200:
					cv2.destroyAllWindows()			# Remover o comentar durante la competencia
					break
		else:

			for qrcode in decode(img):
				
				pts = np.array([qrcode.polygon],np.int32)
				pts = pts.reshape((-1,1,2))
				cv2.polylines(img,[pts],True,(255,0,255),2)
				pts2 = qrcode.rect 
				cv2.putText(img,qrcode.type,(pts2[0],pts2[1]),cv2.FONT_HERSHEY_SIMPLEX,0.9,(255,0,255),2)

		# Remover o comentar esta línea antes de la competencia.
		cv2.imshow(winName,img)
		sleep(0.02)
			

	elif datoModo['detect']:

		# Remover estas dos líneas o comentarlas antes de la competencia.
		winName = "DETECTAR COLOR"
		cv2.namedWindow(winName, cv2.WINDOW_AUTOSIZE)
		color = "desconocido"

		try:
			imgResponse = urllib.request.urlopen(url_img)
			
		except URLError as e:
			print("Error en la URL: Servidor no encontrado " + e)
			break 

		imgNp = np.array(bytearray(imgResponse.read()),dtype=np.uint8)
		img = cv2.imdecode(imgNp,-1)

		img = cv2.rotate(img,cv2.ROTATE_90_CLOCKWISE)

		start_point = (150,250)
		end_point = (450,550)

		img = cv2.rectangle(img,start_point,end_point,(255,0,255),2)

		cv2.imshow(winName,img)
		sleep(0.02)

		colores = {'azul':[159,6,16],'rojo':[55,39,238],'verde':[44,214,68],'naranja':[0,94,254],'amarillo':[0,233,254]}

		img_sm = img[start_point[0]-1:start_point[1],end_point[0]-1:end_point[1]]

		for k, v in colores.items():
			minimo = [i*tl[0] for i in v]
			maximo = [i*tl[1] for i in v]	

			minimo = np.array(minimo, dtype='uint8')
			maximo = np.array(maximo, dtype='uint8')

			mask = cv2.inRange(img_sm, minimo, maximo)		

			prom_detec = np.mean(mask)
			print("Promedio detectado: {0}".format(prom_detec))

			if prom_detec > lim:
				color = k
				print("Color detectado: "+color)
				break

		if color in colores.keys():
			url2 = "http://192.168.4.1/color/{0}".format(color)

			colResponse = urllib.request.urlopen(url2)

			if codResponse.status == 200:
				cv2.destroyAllWindows()

	# Esperamos a que se presione ESC para salir de la ejecución
	tecla = cv2.waitKey(5) & 0xFF
	if tecla == 27:
		break

# Remover o comentar esta línea antes de la competencia.
cv2.destroyAllWindows()


