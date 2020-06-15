from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.common.exceptions import TimeoutException

# TODO: Support Chrome and Firefox
chrome_options = Options()
chrome_options.add_argument("--disable_gpu")
chrome_options.add_argument("--headless")
chrome_options.add_argument("--no-sandbox")

driver = webdriver.Chrome(executable_path="/chromedriver", chrome_options=chrome_options)

start_url = "http://127.0.0.1:8000/udTest.html"
driver.get(start_url)

try:
	element = WebDriverWait(driver, 30).until(EC.text_to_be_present_in_element_value((By.ID, 'output'), ' tests.'))
except TimeoutException:
	print "Loading took too much time!"
except Exception as e:
	print "Something went wrong! ", e

# TODO: Output changes as they come in...
print driver.execute_script("return document.getElementById('output').value;")

exitStatus = driver.execute_script("return EXITSTATUS;")

driver.quit()

exit(exitStatus)
