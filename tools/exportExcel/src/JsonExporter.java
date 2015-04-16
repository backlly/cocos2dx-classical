import java.io.File;
import java.io.IOException;

import org.apache.poi.ss.usermodel.Cell;
import org.apache.poi.ss.usermodel.Row;
import org.apache.poi.ss.usermodel.Sheet;
import org.apache.poi.ss.usermodel.Workbook;

import com.alibaba.fastjson.JSONObject;

public class JsonExporter extends BaseExporter {
	@Override
	public void doExport(Workbook book, Sheet sheet, File file) throws IOException {
		JSONObject json = new JSONObject();
		
		// row num
		int rowNum = sheet.getLastRowNum();

		// field and type row
		Row fieldRow = sheet.getRow(2);
		Row typeRow = sheet.getRow(3);

		String idColName = fieldRow.getCell(0).getStringCellValue();
		idColName = firstCapital(idColName);
		for (int i = 4; i <= rowNum; i++) {
			// json of this row
			JSONObject rowJson = new JSONObject();
			
			// if cell is empty, skip
			int colNum = fieldRow.getLastCellNum();
			Row row = sheet.getRow(i);
			if(row == null)
				continue;
			Cell cell0 = row.getCell(0);
			if (cell0 == null)
				continue;
			
			// put row
			// if not numeric and value is empty, skip
			if (row.getCell(0).getCellType() == Cell.CELL_TYPE_NUMERIC) {
				rowJson.put(idColName + "", row.getCell(0).getNumericCellValue());
				json.put("" + rowJson.getIntValue(idColName), rowJson);
			} else {
				String sValue = row.getCell(0).getStringCellValue().trim();
				if("".equals(sValue))
					continue;
				rowJson.put(idColName, sValue);
				json.put(rowJson.getString(idColName), rowJson);
			}

			for (int j = 1; j < colNum; j++) {
				if (fieldRow.getCell(j) == null || fieldRow.getCell(j).getCellType() == Cell.CELL_TYPE_BLANK)
					continue;
				
				// col name and quick reject empty col
				String colName = fieldRow.getCell(j).getStringCellValue();
				if (colName == null || "".equals(colName))
					continue;
				colName = firstCapital(colName);
				
				// get type and quick reject empty type
				String type = typeRow.getCell(j).getStringCellValue();
				if (type == null || "".equals(type))
					continue;
				
				// get cell and check its type
				Cell cell = row.getCell(j);
				if (cell == null || cell.getCellType() == Cell.CELL_TYPE_BLANK) {
					if (type.equalsIgnoreCase("int") || type.equalsIgnoreCase("Byte") || type.equalsIgnoreCase("float") || type.equalsIgnoreCase("bool")) {
						rowJson.put(colName, 0);
					} else {
						rowJson.put(colName, "");
					}
				} else if (cell.getCellType() == Cell.CELL_TYPE_STRING) {
					if(type.equalsIgnoreCase("luafunc")) {
						String lua = cell.getStringCellValue();
						lua = lua.replace("\n", "\\n");
						lua = lua.replace("\r", "\\r");
						rowJson.put(colName, lua);
					} else {
						rowJson.put(colName, cell.getStringCellValue());
					}
				} else if (cell.getCellType() == Cell.CELL_TYPE_NUMERIC) {
					rowJson.put(colName, cell.getNumericCellValue());
				} else if(cell.getCellType() == Cell.CELL_TYPE_BOOLEAN) {
					rowJson.put(colName, cell.getBooleanCellValue() ? "true" : "false");
				} else { // 公式
					rowJson.put(colName, cell.getStringCellValue());
				}
			}
		}
		
		// generate json
		String className = "X" + firstCapital(sheet.getSheetName());
		String jsonPath = new File(file.getParentFile(), className + ".json").getAbsolutePath();
		writeFile(jsonPath, json.toJSONString());
	}
}